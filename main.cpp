#define SYSTEM_WINDOWS

#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "std/zip.hpp"
#include "std/span.hpp"
#include "std/file.hpp"
#include "math/math2d.hpp"
#include "math/math3d.hpp"
#include <float.h> 

#ifdef SYSTEM_WINDOWS
	#include <windows.h>
#endif

enum PROGRAM_OPTIONS {
	OPTION_NONE                   = 0x0,
	OPTION_INFO                   = 0x1,
	OPTION_INFO_FULL              = 0x2,
	OPTION_RECURSIVE              = 0x4,
	OPTION_SINGLELOG              = 0x8,
	OPTION_TRUNCATE               = 0x10,
	OPTION_MERGE_POINTS           = 0x20,
	OPTION_MERGE_POINTS_SELECTIVE = 0x40,
	OPTION_ONLY_USER_VALUE        = 0x80
};

constexpr static uint32_t signature_mlod = 0x444f4c4d;
constexpr static uint32_t signature_tagg = 0x47474154;
constexpr static uint32_t signature_sp3x = 0x58335053;
constexpr static uint32_t signature_odol = 0x4c4f444f;

struct GLOBAL_VARIABLES {
	int files_ok;
	int files_total;
	std::vector<std::string> files_to_skip;
} global = {
	0,
	0
};

constexpr bool starts_with(std::string_view sv, std::string_view prefix) noexcept {
	return (sv.size() >= prefix.size() && sv.substr(0, prefix.size()) == prefix);
}
// based on https://community.bistudio.com/wiki/Compressed_LZSS_File_Format
bool Decode(fp::span<std::byte> out, fp::file& file) {
	int32_t outPos = 0;
	uint32_t sum = 0u;
	uint32_t flags = 0u;
	size_t size = out.size();
	while (size != 0) {
		flags >>= 1;
		if ((flags & 0x100) == 0) {
			flags = file.getc() | 0xff00;
			if (file.error() || file.eof()) {
				return false;
			}
		}
		if (flags & 0x01u) {
			// raw data
			const auto data = static_cast<uint8_t>(file.getc());
			if (file.error() || file.eof()) {
				return false;
			}
			sum += data;
			out[outPos] = static_cast<std::byte>(data);
			++outPos;
			--size;
		} else {
			int32_t rpos = static_cast<int32_t>(file.getc());
			int32_t rlen = static_cast<int32_t>(file.getc());
			if (file.error() || file.eof()) {
				return false;
			}
			rpos |= (rlen & 0xf0) << 4;
			rlen &= 0x0f;
			rlen += 3;
			while (rpos > outPos && rlen != 0u) {
				// special case space fill
				sum += 0x20;
				out[outPos] = static_cast<std::byte>(0x20);
				++outPos;
				--size;
				if (size == 0u) {
					break;
				}
				--rlen;
			}
			rpos = outPos - rpos;
			auto from = &out[rpos];
			auto to = &out[outPos];
			outPos += rlen;
			for (; rlen > 0; --rlen) {
				const auto data = *from;
				++from;
				sum += std::to_integer<uint8_t>(data);
				*to = data;
				++to;
				--size;
				if (size == 0u) {
					break;
				}
			}
		}
	}
	uint32_t checkSum;
	if (file.read(fp::to_writable_bytes(checkSum)) != sizeof(checkSum)) {
		return false;
	}
	return (checkSum == sum);
}
template <class T>
constexpr inline bool has_load(
	int, std::enable_if_t<sizeof(std::declval<T&>().Load(std::declval<fp::file&>()), bool())>* = 0) {
	return true;
}

template <class Object>
constexpr inline bool has_load(...) {
	return false;
}

template <class T>
T ReadValue(fp::file& file) {
	T ret;
	if constexpr (has_load<T>(0)) {
		ret.Load(file);
	} else {
		file.read(fp::to_writable_bytes(ret));
	}
	return ret;
}

template <class T, class... Args>
void ReadValue(T& value, fp::file& file, Args&&... args) {
	if constexpr (sizeof...(Args) > 0) {
		value.Load(file, std::forward<Args>(args)...);
	} else if constexpr (has_load<T>(0)) {
		value.Load(file);
	} else {
		file.read(fp::to_writable_bytes(value));
	}
}

template <class T>
void SkipValue(fp::file& file) {
	file.skip(sizeof(T));
}

template <>
void ReadValue(std::string& value, fp::file& file) {
	value.clear();
	while (true) {
		const auto c = file.getc();
		if (c=='\0' || c==EOF) {
			break;
		}
		value.push_back(c);
	}
}

template <class T, class... Args>
void ReadValue(std::vector<T>& array, fp::file& file, Args&&... args) {
	uint32_t size = 0;
	file.read(fp::to_writable_bytes(size));
	array.resize(size);

	for (uint32_t index = 0; index < size; index++) {
		ReadValue(array[index], file, std::forward<Args>(args)...);
	}
}

template <class T>
std::vector<T> ReadBinaryArray(fp::file& file) {
	uint32_t size = 0;
	file.read(fp::to_writable_bytes(size));

	std::vector<T> array(size);
	file.read(fp::as_writable_bytes(fp::span(array)));
	return array;
}

template <class T, class... Args>
void ReadArray(std::vector<T>& array, fp::file& file, Args&&... args) {
	uint32_t size = 0;
	file.read(fp::to_writable_bytes(size));

	array.resize(size);

	for (uint32_t index = 0; index < size; index++) {
		ReadValue(array[index], file, std::forward<Args>(args)...);
	}
}

template <class T>
void ReadCompressedArray(std::vector<T>& array, fp::file& file) {
	uint32_t size = 0;
	file.read(fp::to_writable_bytes(size));
	array.resize(size);
	if (array.size() * sizeof(T) < 1024) {
		// no compression for small stuff
		file.read(fp::as_writable_bytes(fp::span(array)));
	} else {
		if (!Decode(fp::as_writable_bytes(fp::span(array)), file)) {
			std::cout << "Failed to decode data" << std::endl;
			exit(1);
		}
	}
}

template <class T, class... Args>
void ReadArraySize(std::vector<T>& array, uint32_t size, fp::file& file, Args&&... args) {
	array.resize(size);
	for (uint32_t i=0; i<size; i++)
		ReadValue(array[i], file, std::forward<Args>(args)...);
}

void ReadValueChar(std::string& value, uint32_t size, fp::file& file) {
	value.clear();
	bool end = false;
	
	for (int i=0; i<size; i++) {
		const auto c = file.getc();
		if (c=='\0' || c==EOF)
			end = true;
		if (!end)
			value.push_back(c);
	}
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

struct ColorBgra {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
};

class ShapeSection {
public:
	void Load(fp::file& file) {
		ReadValue(startIndex, file);
		ReadValue(endIndex, file);
		ReadValue(material, file);
		ReadValue(textureIndex, file);
		ReadValue(special, file);
	}

	uint32_t startIndex;
	uint32_t endIndex;
	int material;

	int16_t textureIndex;
	int special;
};

struct NamedSection {
	void Load(fp::file& file, uint32_t version = 7) {
		ReadValue(name, file);
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);

		ReadCompressedArray(faceIndices, file);
		ReadCompressedArray(faceWeights, file);
		ReadCompressedArray(faceSelectionIndices, file);
		ReadValue(needSelection, file);
		ReadCompressedArray(faceSelectionIndices2, file);
		ReadCompressedArray(vertexIndices, file);
		ReadCompressedArray(vertexWeights, file);
	}

	std::string name;

	std::vector<uint16_t> faceIndices;
	std::vector<uint8_t> faceWeights;

	std::vector<uint32_t> faceSelectionIndices;
	std::vector<uint32_t> faceSelectionIndices2;
	bool needSelection;

	std::vector<uint16_t> vertexIndices;
	std::vector<uint8_t> vertexWeights;
};

struct NamedProperty {
	void Load(fp::file& file) {
		ReadValue(name, file);
		ReadValue(value, file);
	}

	std::string name;
	std::string value;
};

struct AnimationPhase {
	void Load(fp::file& file) {
		ReadValue(time, file);
		ReadArray(points, file);
	}

	float time;
	std::vector<Vector3F> points;
};

struct ProxyObject {
	void Load(fp::file& file) {
		ReadValue(name, file);
		ReadValue(transform, file);
		ReadValue(id, file);
		ReadValue(sectionIndex, file);
	}

	std::string name;
	Matrix4F transform;

	int32_t id;
	int32_t sectionIndex;
};

struct Face {
	Face(uint32_t p_flags, uint32_t p_textureIndex, uint32_t p_offset, uint16_t p_v0, uint16_t p_v1, uint16_t p_v2)
		: flags(p_flags), textureIndex(p_textureIndex), offset(p_offset), v0(p_v0), v1(p_v1), v2(p_v2), v3(p_v0) {}

	Face(uint32_t p_flags, uint32_t p_textureIndex, uint32_t p_offset, uint16_t p_v0, uint16_t p_v1, uint16_t p_v2,
		uint16_t p_v3)
		: flags(p_flags), textureIndex(p_textureIndex), offset(p_offset), v0(p_v0), v1(p_v1), v2(p_v2), v3(p_v3) {
		if (!IsQuad()) {
			std::cout << "Invalid quad" << std::endl;
			exit(1);
		}
	}

	bool IsQuad() const { return (v0 != v3); }
	uint32_t flags;
	uint32_t textureIndex;
	uint32_t offset;
	uint16_t v0;
	uint16_t v1;
	uint16_t v2;
	uint16_t v3;
};

union LodType {
    float graphical;
    uint32_t functional;
};

class LodShape {
public:
	LodShape(fp::file& file) {
		ReadCompressedArray(m_flags, file);
		ReadCompressedArray(m_uv, file);

		ReadArray(m_positions, file);
		ReadArray(m_normals, file);

		ReadValue(m_hintsOr, file);
		ReadValue(m_hintsAnd, file);

		ReadValue(m_min, file);
		ReadValue(m_max, file);

		ReadValue(m_center, file);
		ReadValue(m_radius, file);

		ReadArray(m_textureNames, file);

		ReadCompressedArray(m_pointToVertices, file);
		ReadCompressedArray(m_vertexToPoints, file);
		{
			uint32_t count = 0;
			uint32_t size = 0;
			ReadValue(count, file);
			ReadValue(size, file);
			uint32_t offset = 0;
			m_orignalFaces.reserve(count);
			for (int i = 0; i < count; i++) {
				uint32_t flags;
				ReadValue(flags, file);
				uint16_t textureIndex;
				ReadValue(textureIndex, file);

				uint8_t n;
				ReadValue(n, file);
				if (n == 3) {
					uint16_t v0;
					uint16_t v1;
					uint16_t v2;
					ReadValue(v0, file);
					ReadValue(v1, file);
					ReadValue(v2, file);
					m_orignalFaces.emplace_back(flags, textureIndex, offset, v0, v1, v2);
					offset += 2 + 3 * 2;
				} else if (n == 4) {
					uint16_t v0;
					uint16_t v1;
					uint16_t v2;
					uint16_t v3;
					ReadValue(v0, file);
					ReadValue(v1, file);
					ReadValue(v2, file);
					ReadValue(v3, file);
					m_orignalFaces.emplace_back(flags, textureIndex, offset, v0, v1, v2, v3);
					offset += 2 + 4 * 2;
				} else {
					std::cout << "Invalid face with n " << static_cast<uint32_t>(n) << std::endl;
					exit(1);
				}
			}
		}

		ReadArray(m_sections, file);
		ReadArray(m_namedSections, file);
		ReadArray(m_namedProperties, file);
		ReadArray(m_animationPhases, file);

		ReadValue(m_color, file);
		ReadValue(m_color2, file);
		ReadValue(m_flags2, file);
		ReadArray(m_proxies, file);
	}

	const auto& GetTextureNames() const noexcept { return m_textureNames; }

	const auto& GetPositions() const noexcept { return m_positions; }

	const auto& GetNormals() const noexcept { return m_normals; }
	const auto& GetFlags() const noexcept { return m_flags; }

	const auto& GetUvs() const noexcept { return m_uv; }

	const auto& GetOriginalFaces() const noexcept { return m_orignalFaces; }

	const auto& GetNamedSections() const noexcept { return m_namedSections; }
	const auto& GetNamedProperties() const noexcept { return m_namedProperties; }
	
	const auto& GetPointToVertices() const noexcept { return m_pointToVertices; }
	uint16_t VertexToPoint(uint16_t vertex) const noexcept { return m_vertexToPoints[vertex]; }

//private:
	std::vector<uint32_t> m_flags;
	std::vector<Vector2> m_uv;

	std::vector<Vector3F> m_positions;
	std::vector<Vector3F> m_normals;
	std::vector<Face> m_orignalFaces;

	std::vector<std::string> m_textureNames;

	std::vector<ShapeSection> m_sections;

	std::vector<uint16_t> m_pointToVertices;
	std::vector<uint16_t> m_vertexToPoints;

	std::vector<NamedSection> m_namedSections;
	std::vector<NamedProperty> m_namedProperties;
	std::vector<AnimationPhase> m_animationPhases;
	std::vector<ProxyObject> m_proxies;

	Vector3F m_center;
	float m_radius;

	Vector3F m_min;
	Vector3F m_max;

	uint32_t m_hintsAnd;
	uint32_t m_hintsOr;

	ColorBgra m_color;
	ColorBgra m_color2;
	uint32_t m_flags2;
};

class Shape {
public:
	Shape(fp::file& file) {
		ReadValue(m_version, file);
		ReadValue(m_lodCount, file);

		m_lods.reserve(m_lodCount);
		for (uint32_t lodIndex = 0; lodIndex < m_lodCount; lodIndex++) {
			m_lods.emplace_back(file);
		}

		m_lodDistances.reserve(m_lodCount);
		m_lodDistances.resize(m_lodCount);

		file.read(fp::as_writable_bytes(fp::span(m_lodDistances)));		
		ReadValue(m_properties, file);

		ReadValue(m_lodSphere, file);
		ReadValue(m_physicsSphere, file);

		ReadValue(m_properties2, file);

		ReadValue(m_hintsAnd, file);
		ReadValue(m_hintsOr, file);

		ReadValue(m_aimPoint, file);

		ReadValue(m_color, file);
		ReadValue(m_color2, file);
		ReadValue(m_density, file);

		ReadValue(m_min, file);
		ReadValue(m_max, file);

		ReadValue(m_lodCenter, file);
		ReadValue(m_physicsCenter, file);

		ReadValue(m_massCenter, file);
		ReadValue(m_invInertia, file);

		ReadValue(m_autoCenter, file);
		ReadValue(m_autoCenter2, file);

		ReadValue(m_canOcclude, file);
		ReadValue(m_canBeOccluded, file);
		ReadValue(m_allowAnimation, file);

		ReadValue(m_mapType, file);

		ReadCompressedArray(m_masses, file);

		ReadValue(m_mass, file);
		ReadValue(m_invMass, file);

		ReadValue(m_armor, file);
		ReadValue(m_invArmor, file);

		ReadValue(m_memoryLodIndex, file);
		ReadValue(m_geometryLodIndex, file);
		ReadValue(m_geometryFireLodIndex, file);
		ReadValue(m_geometryViewLodIndex, file);
		ReadValue(m_geometryViewPilotLodIndex, file);
		ReadValue(m_geometryViewGunnerLodIndex, file);
		ReadValue(m_geometryViewCommanderLodIndex, file);
		ReadValue(m_geometryViewCargoLodIndex, file);
		ReadValue(m_landContactLodIndex, file);
		ReadValue(m_roadwayLodIndex, file);
		ReadValue(m_pathsLodIndex, file);
		ReadValue(m_hitpointsLodIndex, file);
	}

	const auto& GetLods() const noexcept { return m_lods; }
	const auto& GetLodDistances() const noexcept { return m_lodDistances; }

	const auto& GetAimPoint() const noexcept { return m_aimPoint; }

	int8_t GetMemoryLodIndex() const noexcept { return m_memoryLodIndex; }
	int8_t GetGeometryLodIndex() const noexcept { return m_geometryLodIndex; }
	int8_t GetGeometryFireLodIndex() const noexcept { return m_geometryFireLodIndex; }
	int8_t GetGeometryViewLodIndex() const noexcept { return m_geometryViewLodIndex; }
	int8_t GetGeometryViewPilotLodIndex() const noexcept { return m_geometryViewPilotLodIndex; }
	int8_t GetGeometryViewGunnerLodIndex() const noexcept { return m_geometryViewGunnerLodIndex; }
	int8_t GetGeometryViewCommanderLodIndex() const noexcept { return m_geometryViewCommanderLodIndex; }
	int8_t GetGeometryViewCargoLodIndex() const noexcept { return m_geometryViewCargoLodIndex; }
	int8_t GetLandContactLodIndex() const noexcept { return m_landContactLodIndex; }
	int8_t GetRoadwayLodIndex() const noexcept { return m_roadwayLodIndex; }
	int8_t GetPathsLodIndex() const noexcept { return m_pathsLodIndex; }
	int8_t GetHitpointsLodIndex() const noexcept { return m_hitpointsLodIndex; }

	const auto& GetCenterOfMass() const noexcept { return m_massCenter; }
	const auto& GetLodCenter() const noexcept { return m_lodCenter; }
	const auto& GetMasses() const noexcept { return m_masses; }

	float GetMass() const noexcept { return m_mass; }

	float GetArmor() const noexcept { return m_armor; }
	float GetDensity() const noexcept { return m_density; }

	uint8_t GetMapType() const noexcept { return m_mapType; }
	ColorBgra GetColor() const noexcept { return m_color; }

//private:
	uint32_t m_version;
	uint32_t m_lodCount;

	std::vector<LodShape> m_lods;
	std::vector<LodType> m_lodDistances;

	uint32_t m_properties;
	uint32_t m_properties2;

	uint32_t m_hintsAnd;
	uint32_t m_hintsOr;

	float m_lodSphere;
	float m_physicsSphere;

	Vector3F m_aimPoint;

	ColorBgra m_color;
	ColorBgra m_color2;

	float m_density;

	Vector3F m_min;
	Vector3F m_max;

	Vector3F m_lodCenter;
	Vector3F m_physicsCenter;

	Vector3F m_massCenter;

	Matrix3F m_invInertia;

	bool m_autoCenter;
	bool m_autoCenter2;

	bool m_canOcclude;
	bool m_canBeOccluded;
	bool m_allowAnimation;

	uint8_t m_mapType;

	std::vector<float> m_masses;

	float m_mass;
	float m_invMass;

	float m_armor;
	float m_invArmor;

	int8_t m_memoryLodIndex;
	int8_t m_geometryLodIndex;
	int8_t m_geometryFireLodIndex;
	int8_t m_geometryViewLodIndex;
	int8_t m_geometryViewPilotLodIndex;
	int8_t m_geometryViewGunnerLodIndex;
	int8_t m_geometryViewCommanderLodIndex;
	int8_t m_geometryViewCargoLodIndex;
	int8_t m_landContactLodIndex;
	int8_t m_roadwayLodIndex;
	int8_t m_pathsLodIndex;
	int8_t m_hitpointsLodIndex;
};

struct PointMLOD {
	Vector3F pos;
	uint32_t flags;
};

struct VertexTableMLOD {
	int32_t points_index;
	int32_t normals_index;
	float u;
	float v;
};
	
struct FaceMLOD {
	std::string texture;
	int32_t type;
	std::vector<VertexTableMLOD> table;
	int32_t flags;
};

struct TaggFrameMLOD {
	float frame_time;
	std::vector<Vector3F> bone_pos;
};

struct MaterialMLOD {
	ColorBgra ambient;
	ColorBgra diffuse;
	ColorBgra specular;
	ColorBgra emissive;
};

struct NamedSelectionMLOD {
	std::vector<uint8_t> point;
	std::vector<uint8_t> face;
};

class LodShapeMLOD {
public:
	LodShapeMLOD(fp::file& file) {
		ReadValue(signature, file);
		
		if (signature == signature_sp3x) {
			ReadValue(major_version, file);
			ReadValue(minor_version, file);
			ReadValue(points_count, file);
			ReadValue(normals_count, file);
			ReadValue(faces_count, file);
			ReadValue(flags, file);
			
			ReadArraySize(points, points_count, file);
			ReadArraySize(normals, normals_count, file);
			
			for (int32_t i=0; i<faces_count; i++) {
				FaceMLOD current_face;
				ReadValueChar(current_face.texture, 32, file);
				ReadValue(current_face.type, file);
				
				current_face.table.resize(4);
				for (int32_t j=0; j<4; j++) {
					ReadValue(current_face.table[j].points_index, file);
					ReadValue(current_face.table[j].normals_index, file);
					ReadValue(current_face.table[j].u, file);
					ReadValue(current_face.table[j].v, file);
				}
				
				ReadValue(current_face.flags, file);
				faces.push_back(current_face);
			}
			
			int32_t current_tag_signature;
			ReadValue(current_tag_signature, file);

			while (current_tag_signature == signature_tagg) {
				std::string tag_name;
				ReadValueChar(tag_name, 64, file);
				tags.push_back(tag_name);
				int32_t tag_size;
				ReadValue(tag_size, file);
				
				if (tag_name == "#EndOfFile#") {
					break;
				} else
				if (tag_name == "#SharpEdges#") {
					ReadArraySize(sharp_edges, tag_size/4, file);
				} else
				if (tag_name == "#Property#") {
					std::string val;
					ReadValueChar(val, 64, file);
					properties.push_back(val);
					ReadValueChar(val, 64, file);
					properties.push_back(val);
				} else
				if (tag_name == "#Mass#") {
					ReadArraySize(mass, points_count, file);
				} else
				if (tag_name == "#Animation#") {
					TaggFrameMLOD current;
					ReadValue(current.frame_time, file);
					uint32_t bones_count = (tag_size - sizeof(current.frame_time)) / sizeof(Vector3F);
					current.bone_pos.reserve(bones_count);
					ReadArraySize(current.bone_pos, bones_count, file);
					animations.push_back(current);
				} else
				if (tag_name == "#MaterialIndex#") {
					for (int i=0; i<tag_size/4; i++) {
						MaterialMLOD current;
						ReadValue(current.diffuse, file);
						ReadValue(current.ambient, file);
						ReadValue(current.specular, file);
						ReadValue(current.emissive, file);
						materials.push_back(current);
					}
				} else
				if (tag_name[0] != '#') {
					NamedSelectionMLOD current;
					if ((points_count+faces_count) == tag_size) {
						current.point.reserve(points_count);
						current.point.reserve(faces_count);
						ReadArraySize(current.point, points_count, file);
						ReadArraySize(current.face, faces_count, file);
					} else {
						current.point.resize(0);
						current.point.resize(0);
						file.skip(tag_size);
					}
					named_selections.push_back(current);
				} else {
					file.skip(tag_size);
				}
			}
			
			ReadValue(resolution, file);
		}
	}

	int32_t signature;
	int32_t major_version;
	int32_t minor_version;
	int32_t points_count;
	int32_t normals_count;
	int32_t faces_count;
	int32_t flags;
	
	std::vector<PointMLOD> points;
	std::vector<Vector3F> normals;
	std::vector<FaceMLOD> faces;
	std::vector<std::string> tags;
	std::vector<uint32_t> sharp_edges;
	std::vector<std::string> properties;
	std::vector<TaggFrameMLOD> animations;
	std::vector<MaterialMLOD> materials;
	std::vector<float> mass;
	std::vector<NamedSelectionMLOD> named_selections;
	
	LodType resolution;
};

class ShapeMLOD {
public:
	ShapeMLOD(fp::file& file) {
		ReadValue(version, file);
		ReadValue(lod_count, file);
		
		lods.reserve(lod_count);
		for (uint32_t lodIndex=0; lodIndex<lod_count; lodIndex++) {
			lods.emplace_back(file);
			
			if (lods[lodIndex].signature != signature_sp3x)
				break;
		}
		
		ReadValueChar(SP3X_DefaultPath, 32, file);
	}
	
	uint32_t version;
	uint32_t lod_count;
	std::vector<LodShapeMLOD> lods;
	std::string SP3X_DefaultPath;
};

struct FaceVertex {
	uint32_t vertexIndex;
	uint32_t normalIndex;
	Vector2 uv;
};

template <size_t N>
void WriteName(fp::file& f, std::string_view name) {
	if (name.size() >= N) {
		name = name.substr(0, N - 1);
	}
	f.write(name);
	for (size_t i = name.size(); i < N; ++i) {
		f.write(fp::to_bytes('\0'));
	}
}

static std::string CreateOutPath(std::string_view inPath, std::string suffix="_mlod.p3d") noexcept {
	const auto extPos = inPath.rfind('.');
	if (extPos == std::string_view::npos) {
		return std::string(inPath) + suffix;
	}
	return std::string(inPath.substr(0, extPos)) + suffix;
}

std::string FormatLodType(LodType lod) {
	switch (lod.functional) {
		case 0x447a0000 : return "View - Gunner";
		case 0x44898000 : return "View - Pilot";
		case 0x44960000 : return "View - Cargo";
		case 0x551184e7 : return "Geometry";
		case 0x58635fa9 : return "Memory";
		case 0x58e35fa9 : return "LandContact";
		case 0x592a87bf : return "RoadWay";
		case 0x59635fa9 : return "Paths";
		case 0x598e1bca : return "Hit-points";
		case 0x59aa87bf : return "View Geometry";
		case 0x59c6f3b4 : return "Fire Geometry";
		case 0x59e35fa9 : return "View - Cargo - Geometry";
		case 0x59ffcb9e : return "View - Cargo - Fire Geometry";
		case 0x5a0e1bca : return "View - Commander";
		case 0x5a1c51c4 : return "View - Commander - Geometry";
		case 0x5a2a87bf : return "View - Commander - Fire Geometry";
		case 0x5a38bdb9 : return "View - Pilot - Geometry";
		case 0x5a46f3b4 : return "View - Pilot - Fire Geometry";
		case 0x5a5529af : return "View - Gunner - Geometry";
		case 0x5a635fa9 : return "View - Gunner - Fire Geometry";
		default : return std::to_string(lod.graphical);
	}
}

std::string FormatSignature(uint32_t number) {
	std::string name(4, ' ');
	
	for (int i=3, shift=24;  i>=0;  i--, shift-=8) {
		name[i] = static_cast<char>((number >> shift) & 0xFF);
		
		if (!std::isgraph(name[i])) {
			std::stringstream ss;
			ss << "0x" << std::hex << number;
			return ss.str();
		}
	}

	return name;
}

uint32_t convert_point_light_flags(uint32_t flags_odol, bool only_user_value) {
	uint32_t flags_mlod = 0u;
	
	if (!only_user_value) {
		if (flags_odol >> 20 == 0xC8) flags_mlod|=0x10; //Shining
		if (flags_odol >> 20 == 0xC9) flags_mlod|=0x20; //Always in shadow
		if (flags_odol >> 20 == 0xCA) flags_mlod|=0x80; //Half lighted
		if (flags_odol >> 20 == 0xCB) flags_mlod|=0x40; //Fully lighted
	}
	
	if (flags_mlod == 0)
		flags_mlod |= (flags_odol >> 4) & 0xFF0000; //User value
	
	flags_mlod |= (flags_odol >> 8) & 0xF; //Surface
	flags_mlod |= (flags_odol >> 4) & 0x300; //Decal
	flags_mlod |= (flags_odol >> 2) & 0x3000; //Fog
	
	return flags_mlod;
}

int Parse_P3D(std::string filename_input, std::string file_info, int &options) {
	std::cout << filename_input << std::endl;
	fp::file file(filename_input, "rb");
	
	if (!file.is_open()) {
		std::cout << "Failed to open - error " << errno << ": " << strerror(errno) << std::endl;
		return 1;
	}
	
	// Verify file type
	uint32_t current_file_signature;
	ReadValue(current_file_signature, file);
	
	if (current_file_signature!=signature_odol  &&  current_file_signature!=signature_mlod) {
		file.close();
		std::cout << "Incorrect file type " << FormatSignature(current_file_signature) << std::endl;
		return 2;
	}
	
	// Create output
	std::fstream out;
	std::ios_base::openmode mode = std::ios::out | std::ios::trunc;
	std::string filename_output  = "";
	
	if (options & OPTION_INFO) {
		filename_output = CreateOutPath(filename_input, ".txt");
		
		if (options & (OPTION_SINGLELOG)) {
			filename_output = "odol2mlod.txt";
			
			if (options & OPTION_TRUNCATE)
				options &= ~OPTION_TRUNCATE;
			else {
				mode |= std::ios::app;
				mode &= ~std::ios::trunc;
			}
		}
	} else
		if (current_file_signature == signature_odol)
			filename_output = CreateOutPath(filename_input);

	if (!filename_output.empty()) {
		out.open(filename_output.c_str(), mode);
		
		if (!out.is_open()) {
			std::cout << "Failed to create file " << filename_output << " - error " << errno << ": " << strerror(errno) << std::endl;
			return 3;
		}
	}
	
	global.files_ok++;
	
	if (options & (OPTION_SINGLELOG)  &&  ~mode & std::ios::trunc)
		out << std::endl << std::endl << "====================================" << std::endl << std::endl;
	
	// Parse input
	if (current_file_signature == signature_odol) {
		Shape shape(file);
		
		if (options & OPTION_INFO) {
			out << 
			filename_input << " - " << file_info << std::endl << std::endl <<
			"Signature: "<< FormatSignature(current_file_signature) << std::endl << 
			"Version: " << shape.m_version << std::endl <<
			"Number of LODs: " << shape.m_lodCount << std::endl << 
			"Flags: " << std::hex << "0x" << shape.m_properties << std::endl;
			
			for (int i=1; options & OPTION_INFO_FULL && i<0x80000000; i*=2)
				if (shape.m_properties & i) 
					out << "\t 0x" << i << std::endl;
			
			out << std::dec <<
			"Bounding sphere radius: " << shape.m_lodSphere << std::endl << 
			"Bounding geometry sphere radius: " << shape.m_physicsSphere << std::endl << 
			"Remarks: " << shape.m_properties2 << std::endl << 
			"Hints and: " << shape.m_hintsAnd << std::endl << 
			"Hints or: " << shape.m_hintsOr << std::endl << 
			"Aim point x:" << shape.m_aimPoint.X() << " y:" << shape.m_aimPoint.Y() << " z:" << shape.m_aimPoint.Z() << std::endl << 
			"Color:" << (int)shape.m_color.r << " g:" << (int)shape.m_color.g << " b:" << (int)shape.m_color.b << " a:" << (int)shape.m_color.a << std::endl << 
			"Color top r:" << (int)shape.m_color2.r << " g:" << (int)shape.m_color2.g << " b:" << (int)shape.m_color2.b << " a:" << (int)shape.m_color2.a << std::endl << 
			"View density: " << shape.m_density << std::endl << 
			"Bounding sphere min x:" << shape.m_min.X() << " y:" << shape.m_min.Y() << " z:" << shape.m_min.Z() << std::endl << 
			"Bounding sphere max x:" << shape.m_max.X() << " y:" << shape.m_max.Y() << " z:" << shape.m_max.Z() << std::endl << 
			"Bounding sphere center x:" << shape.m_lodCenter.X() << " y:" << shape.m_lodCenter.Y() << " z:" << shape.m_lodCenter.Z() << std::endl << 
			"Bounding geometry sphere center x:" << shape.m_physicsCenter.X() << " y:" << shape.m_physicsCenter.Y() << " z:" << shape.m_physicsCenter.Z() << std::endl << 
			"Mass center x:" << shape.m_massCenter.X() << " y:" << shape.m_massCenter.Y() << " z:" << shape.m_massCenter.Z() << std::endl << 
			"Inverse inertia tensor a:" << shape.m_invInertia.Aside().Size() << " u:" << shape.m_invInertia.Up().Size() << " d:" << shape.m_invInertia.Direction().Size() << std::endl << 
			"Auto center: " << shape.m_autoCenter << std::endl << 
			"Lock auto center: " << shape.m_autoCenter2 << std::endl << 
			"Can occlude: " << shape.m_canOcclude << std::endl << 
			"Can be occluded: " << shape.m_canBeOccluded << std::endl << 
			"Allow animation: " << shape.m_allowAnimation << std::endl << 
			"Map type: " << (int)shape.m_mapType << std::endl << 
			"Mass array count: " << shape.m_masses.size() << std::endl;
			
			for (int j=0; j<shape.m_masses.size() && options & OPTION_INFO_FULL; j++)
				out << "\t" << j << " - " << shape.m_masses[j] << std::endl;
				
			out << 
			"Mass: " << shape.m_mass << std::endl << 
			"Mass inverse: " << shape.m_invMass << std::endl << 
			"Armor: " << shape.m_armor << std::endl << 
			"Armor inverse: " << shape.m_invArmor << std::endl << 
			"Memory LOD index: " << (int)shape.m_memoryLodIndex << std::endl << 
			"Geometry LOD index: " << (int)shape.m_geometryLodIndex << std::endl << 
			"Fire geometry LOD index: " << (int)shape.m_geometryFireLodIndex << std::endl << 
			"View geometry LOD index: " << (int)shape.m_geometryViewLodIndex << std::endl << 
			"View pilot geometry LOD index: " << (int)shape.m_geometryViewPilotLodIndex << std::endl << 
			"View gunner geometry LOD index: " << (int)shape.m_geometryViewGunnerLodIndex << std::endl << 
			"View command geometry LOD index: " << (int)shape.m_geometryViewCommanderLodIndex << std::endl << 
			"View cargo geometry LOD index: " << (int)shape.m_geometryViewCargoLodIndex << std::endl << 
			"Land contact LOD index: " << (int)shape.m_landContactLodIndex << std::endl << 
			"Roadway LOD index: " << (int)shape.m_roadwayLodIndex << std::endl << 
			"Paths LOD index: " << (int)shape.m_pathsLodIndex << std::endl << 
			"Hitpoints LOD index: " << (int)shape.m_hitpointsLodIndex;
			
			for (int i=0; i<shape.m_lods.size(); i++) {
				LodShape *l = &shape.m_lods[i];
				
				out << std::endl << std::endl << 
				"LOD: " << FormatLodType(shape.m_lodDistances[i]) << std::endl <<
				"Flags count: " << l->m_flags.size() << std::endl;
				
				for (int j=0; j<l->m_flags.size() && options & OPTION_INFO_FULL; j++)
					out << "\t" << j << " - 0x" << std::hex << l->m_flags[j] << std::dec << std::endl;

				out << 
				"UV: " << l->m_uv.size() << std::endl;
				
				for (int j=0; j<l->m_uv.size() && options & OPTION_INFO_FULL; j++)
					out << "\t" << j << " - u:" << l->m_uv[j].X() << " v:" << l->m_uv[j].Y() << std::endl;
				
				out <<
				"Points: " << l->m_positions.size() << std::endl;
				
				for (int j=0; j<l->m_positions.size() && options & OPTION_INFO_FULL; j++)
					out << "\t" << j << " - x:" << l->m_positions[j].X() << " y:" << l->m_positions[j].Y() << " z:" << l->m_positions[j].Z() << std::endl;
					
				out << 
				"Normals: " << l->m_normals.size() << std::endl;
				
				for (int j=0; j<l->m_normals.size() && options & OPTION_INFO_FULL; j++)
					out << "\t" << j << " - x:" << l->m_normals[j].X() << " y:" << l->m_normals[j].Y() << " z:" << l->m_normals[j].Z() << std::endl;
					
				out << 
				"Hints or: " << l->m_hintsOr << std::endl	<< 
				"Hints and: " << l->m_hintsAnd << std::endl << 
				"Min pos x:" << l->m_min.X() << " y:" << l->m_min.Y() << " z:" << l->m_min.Z() << std::endl << 
				"Max pos x:" << l->m_max.X() << " y:" << l->m_max.Y() << " z:" << l->m_max.Z() << std::endl << 
				"Center pos x:" << l->m_center.X() << " y:" << l->m_center.Y() << " z:" << l->m_center.Z() << std::endl << 
				"Radius: " << l->m_radius << std::endl << 
				"Textures: " << l->m_textureNames.size() << std::endl;
				
				for (int j=0; j<l->m_textureNames.size(); j++)
					out << "\t" << l->m_textureNames[j] << std::endl;
				
				out << 
				"Point to vertice index: " << l->m_pointToVertices.size() << std::endl;
				
					for (int j=0; j<l->m_pointToVertices.size() && options & OPTION_INFO_FULL; j++)
						out << "\t" << j << " " << l->m_pointToVertices[j] << std::endl;
				
				out << "Vertice to point index: " << l->m_vertexToPoints.size() << std::endl;

					for (int j=0; j<l->m_vertexToPoints.size() && options & OPTION_INFO_FULL; j++)
						out << "\t" << j << " " << l->m_vertexToPoints[j] << std::endl;				
				
				out << "Faces: " << l->m_orignalFaces.size() << std::endl;
				
				for (int j=0; j<l->m_orignalFaces.size() && options & OPTION_INFO_FULL; j++)
					out << "\t" << j << " - flags:0x" << std::hex << l->m_orignalFaces[j].flags << std::dec << std::endl;
					
				out << 
				"Sections: " << l->m_sections.size() << std::endl;
				
				for (int j=0; j<l->m_sections.size() && options & OPTION_INFO_FULL; j++)
					out << "\t" << j << " - startIndex:" << l->m_sections[j].startIndex << " endIndex:" << l->m_sections[j].endIndex << " material:" << l->m_sections[j].material << " textureIndex:" << l->m_sections[j].textureIndex << " special:0x" << std::hex << l->m_sections[j].special << std::dec << std::endl;
	
				out << 
				"Named sections: " << l->m_namedSections.size() << std::endl;
				
				for (int j=0; j<l->m_namedSections.size(); j++)
					out << "\t" << l->m_namedSections[j].name << std::endl;
					
				out << "Properties: " << l->m_namedProperties.size()  << std::endl;
				
				for (int j=0; j<l->m_namedProperties.size(); j++)
					out << "\t" << l->m_namedProperties[j].name << "=" << l->m_namedProperties[j].value << std::endl;
					
				out << 
				"Animation frames: " << l->m_animationPhases.size() << std::endl;
				
				for (int j=0; j<l->m_animationPhases.size() && options & OPTION_INFO_FULL; j++) {
					out << "\t" << j << " - Time:" << l->m_animationPhases[j].time << std::endl;
					
					for (int k=0; k<l->m_animationPhases[j].points.size(); k++) {
						out << "\t\t"
						" x:" << l->m_animationPhases[j].points[k].X() << 
						" y:" << l->m_animationPhases[j].points[k].Y() << 
						" z:" << l->m_animationPhases[j].points[k].Z() << 
						std::endl;
					}
				}

				out << 
				"Color top r:" << (int)l->m_color.r << " g:" << (int)l->m_color.g << " b:" << (int)l->m_color.b << " a:" << (int)l->m_color.a << std::endl << 
				"Color r:" << (int)l->m_color2.r << " g:" << (int)l->m_color2.g << " b:" << (int)l->m_color2.b << " a:" << (int)l->m_color2.a << std::endl << 
				"Flags: " << std::hex << "0x" << (int)l->m_flags2 << std::endl;

				for (int j=1; options & OPTION_INFO_FULL && j<0x80000000; j*=2)
					if (l->m_flags2 & j) 
						out << "\t 0x" << j << std::endl;
					
				out << std::dec <<
				"Proxies: " << l->m_proxies.size();
				
				for (int j=0; j<l->m_proxies.size(); j++)
					out << std::endl << "\t" << l->m_proxies[j].name;
			}
		} 
		else {
			fp::file out(filename_output.c_str(), "wb");
			global.files_to_skip.push_back(filename_output);
			
			// P3DHeader
			constexpr static uint32_t mlodVersion = 0x0101;
			out.write(fp::to_bytes(signature_mlod));
			out.write(fp::to_bytes(mlodVersion));
			out.write(fp::to_bytes(static_cast<uint32_t>(shape.GetLods().size())));

			for (auto [lod, lodDistance, lodIndex] : fp::zip_index(shape.GetLods(), shape.GetLodDistances())) {
				uint32_t normalCount = 0u;
				for (const auto& face : lod.GetOriginalFaces()) {
					if (face.IsQuad()) {
						normalCount += 4u;
					} else {
						normalCount += 3u;
					}
				}

				bool merge_this_lod = options & OPTION_MERGE_POINTS || (options & OPTION_MERGE_POINTS_SELECTIVE && lodDistance.graphical>=1000.0f);
				
				// MLOD_LOD
				out.write(fp::to_bytes(signature_sp3x));
				out.write(fp::to_bytes(static_cast<uint32_t>(0x1c)));
				out.write(fp::to_bytes(static_cast<uint32_t>(0x99)));
				uint16_t positonsCount = merge_this_lod ? lod.GetPointToVertices().size() : lod.GetPositions().size();
				out.write(fp::to_bytes(static_cast<uint32_t>(positonsCount)));
				out.write(fp::to_bytes(normalCount));
				out.write(fp::to_bytes(static_cast<uint32_t>(lod.GetOriginalFaces().size())));
				out.write(fp::to_bytes(static_cast<uint32_t>(0x00)));

				// Points
				if (merge_this_lod) {
					for (auto index : lod.GetPointToVertices()) {
						const auto& vertex = lod.GetPositions()[index];
						const auto& flags_odol = lod.GetFlags()[index];
						out.write(fp::to_bytes(vertex + shape.GetLodCenter()));
						out.write(fp::to_bytes(convert_point_light_flags(flags_odol, options & OPTION_ONLY_USER_VALUE)));
					}
				} else {
					for (auto [vertex, flags_odol] : fp::zip(lod.GetPositions(), lod.GetFlags())) {
						out.write(fp::to_bytes(vertex + shape.GetLodCenter()));
						out.write(fp::to_bytes(convert_point_light_flags(flags_odol, options & OPTION_ONLY_USER_VALUE)));
					}
				}

				// Normals
				for (const auto& face : lod.GetOriginalFaces()) {
					out.write(fp::to_bytes(lod.GetNormals()[face.v0]));
					out.write(fp::to_bytes(lod.GetNormals()[face.v1]));
					out.write(fp::to_bytes(lod.GetNormals()[face.v2]));
					if (face.IsQuad()) {
						out.write(fp::to_bytes(lod.GetNormals()[face.v3]));
					}
				}

				// Faces
				uint32_t normalIndex = 0u;
				for (const auto& face : lod.GetOriginalFaces()) {
					std::array<char, 32u> texture;
					texture.fill('\0');
					if (face.textureIndex < lod.GetTextureNames().size()) {
						fp::span_copy(lod.GetTextureNames()[face.textureIndex], texture);
						texture.back() = '\0';
					}
					out.write(texture);

					out.write(fp::to_bytes(static_cast<uint32_t>(face.IsQuad() ? 4u : 3u)));
					std::array<FaceVertex, 4u> vertices;
					if (face.IsQuad()) {
						vertices[0].vertexIndex = merge_this_lod ? lod.VertexToPoint(face.v1) : face.v1;
						vertices[0].normalIndex = normalIndex;
						vertices[0].uv = lod.GetUvs()[face.v1];
						++normalIndex;
						vertices[1].vertexIndex = merge_this_lod ? lod.VertexToPoint(face.v0) : face.v0;
						vertices[1].normalIndex = normalIndex;
						vertices[1].uv = lod.GetUvs()[face.v0];
						++normalIndex;
						vertices[2].vertexIndex = merge_this_lod ? lod.VertexToPoint(face.v3) : face.v3;
						vertices[2].normalIndex = normalIndex;
						vertices[2].uv = lod.GetUvs()[face.v3];
						++normalIndex;
						vertices[3].vertexIndex = merge_this_lod ? lod.VertexToPoint(face.v2) : face.v2;
						vertices[3].normalIndex = normalIndex;
						vertices[3].uv = lod.GetUvs()[face.v2];
						++normalIndex;
					} else {
						vertices[0].vertexIndex = merge_this_lod ? lod.VertexToPoint(face.v1) : face.v1;
						vertices[0].normalIndex = normalIndex;
						vertices[0].uv = lod.GetUvs()[face.v1];
						++normalIndex;
						vertices[1].vertexIndex = merge_this_lod ? lod.VertexToPoint(face.v0) : face.v0;
						vertices[1].normalIndex = normalIndex;
						vertices[1].uv = lod.GetUvs()[face.v0];
						++normalIndex;
						vertices[2].vertexIndex = merge_this_lod ? lod.VertexToPoint(face.v2) : face.v2;
						vertices[2].normalIndex = normalIndex;
						vertices[2].uv = lod.GetUvs()[face.v2];
						++normalIndex;
					}
					out.write(vertices);
					

					// Flags
					uint32_t flags_mlod = 0u;
					
					if ((face.flags & 0x40) != 0u) flags_mlod|=0x8;	//?
					if ((face.flags & 0x20) != 0u) flags_mlod|=0x10; //shadow off

					if ((face.flags & 0x4000000) != 0u) flags_mlod|=0x100;//zbias low
					if ((face.flags & 0x8000000) != 0u) flags_mlod|=0x200;//zbias middle
					if ((face.flags & 0xC000000) != 0u) flags_mlod|=0x300;//zbias high
					if ((face.flags & 0x20000000) != 0u) flags_mlod|=0x1000000;//texture merging off
					out.write(fp::to_bytes(flags_mlod));
				}

				out.write(fp::to_bytes(signature_tagg));

				// Named sections
				{
					const auto namedSectionSize = static_cast<uint32_t>(positonsCount + lod.GetOriginalFaces().size());
					std::vector<uint8_t> sectionWeights;
					sectionWeights.resize(positonsCount);
					std::vector<uint8_t> isFaceInSection;
					isFaceInSection.resize(lod.GetOriginalFaces().size());

					for (const auto& sec : lod.GetNamedSections()) {
						WriteName<64>(out, sec.name);
						out.write(fp::to_bytes(namedSectionSize));

						std::fill(sectionWeights.begin(), sectionWeights.end(), 0u);
						if (sec.vertexWeights.empty()) {
							for (auto index : sec.vertexIndices) {
								sectionWeights[merge_this_lod ? lod.VertexToPoint(index) : index] = 0x01;
							}
						} else {
							for (auto [weight, index] : fp::zip(sec.vertexWeights, sec.vertexIndices)) {								
								//std::cout << index << ' ' << weight << '\n';
								sectionWeights[merge_this_lod ? lod.VertexToPoint(index) : index] = -weight; // why
							}
						}
						
						std::fill(isFaceInSection.begin(), isFaceInSection.end(), 0u);
						for (auto faceIndex : sec.faceIndices) {
							isFaceInSection[faceIndex] = 1u;
						}
						
						out.write(sectionWeights);
						out.write(isFaceInSection);
					}
				}

				// Properties
				for (const auto& prop : lod.GetNamedProperties()) {
					WriteName<64>(out, "#Property#");
					out.write(fp::to_bytes(static_cast<uint32_t>(128)));
					WriteName<64>(out, prop.name);
					WriteName<64>(out, prop.value);
				}

				// Mass
				if (lodIndex == shape.GetGeometryLodIndex() && !shape.GetMasses().empty()) {
					WriteName<64>(out, "#Mass#");
					
					if (merge_this_lod) {
						out.write(fp::to_bytes(static_cast<uint32_t>(4u * shape.GetMasses().size())));
						out.write(shape.GetMasses());
					} else {
						if (shape.GetMasses().size() == lod.GetPositions().size()) {
							out.write(fp::to_bytes(static_cast<uint32_t>(4u * shape.GetMasses().size())));
							out.write(shape.GetMasses());
						} else {
							std::vector<float> pointVertexCounts(lod.m_pointToVertices.size(), 0.0f);
							for (auto pointIndex: lod.m_vertexToPoints) {
								pointVertexCounts[pointIndex] += 1.0f;
							}
							std::vector<float> newMasses;
							newMasses.reserve(lod.m_positions.size());
							for (size_t vertexIndex = 0u;vertexIndex < lod.m_positions.size();++vertexIndex) {
								const auto pointIndex  = lod.m_vertexToPoints[vertexIndex];
								const auto vextexCount = pointVertexCounts[pointIndex];
								newMasses.push_back(shape.GetMasses()[pointIndex] / vextexCount);
							}
							out.write(fp::to_bytes(static_cast<uint32_t>(4u * newMasses.size())));
							out.write(newMasses);
						}
					}
				}
				
				// Animations
				for (int j=0; j<lod.m_animationPhases.size(); j++) {
					WriteName<64>(out, "#Animation#");
					uint32_t tagg_size = sizeof(lod.m_animationPhases[j].time) + 3u * lod.m_animationPhases[j].points.size() * sizeof(float);
					out.write(fp::to_bytes(static_cast<uint32_t>(tagg_size)));
					out.write(fp::to_bytes(lod.m_animationPhases[j].time));
					
					for (int k=0; k<lod.m_animationPhases[j].points.size(); k++) {
						out.write(fp::to_bytes(lod.m_animationPhases[j].points[k].X()));
						out.write(fp::to_bytes(lod.m_animationPhases[j].points[k].Y()));
						out.write(fp::to_bytes(lod.m_animationPhases[j].points[k].Z()));
					}
				}

				// Close LOD
				WriteName<64>(out, "#EndOfFile#");
				out.write(fp::to_bytes(static_cast<uint32_t>(0)));
				out.write(fp::to_bytes(lodDistance));
			}
		}
	} 
	else 
	if (current_file_signature == signature_mlod) {
		if (options & OPTION_INFO) {
			ShapeMLOD shape(file);

			out << 
			filename_input << " - " << file_info << std::endl << std::endl <<
			"Signature: "<< FormatSignature(current_file_signature) << std::endl << 
			"Version: " << shape.version << std::endl <<
			"Number of LODs: " << shape.lod_count << std::endl <<
			"SP3X default path: " << shape.SP3X_DefaultPath << std::endl;
			
			for (int i=0; i<shape.lods.size(); i++) {
				LodShapeMLOD *l = &shape.lods[i];
				
				if (l->signature != signature_sp3x) {
					out << "LOD format " << FormatSignature(l->signature) << " not supported" << std::endl;
					break;
				}
				
				bool animation_done  = false;
				bool property_done   = false;
				int nselection_index = 0;
				
				out << std::endl <<
				"LOD: " << FormatLodType(l->resolution) << std::endl <<
				"Signature: " << FormatSignature(l->signature) << std::endl << 
				"Major version: " << l->major_version << std::endl << 
				"Minor version: " << l->minor_version << std::endl << 
				"Points: " << l->points.size() << std::endl;
				
				for (int j=0; j<l->points.size() && options & OPTION_INFO_FULL; j++)
					out << "\t" << j << " - x:" << l->points[j].pos.X() << " y:" << l->points[j].pos.Y() << " z:" << l->points[j].pos.Z() << " flags:0x" << std::hex << l->points[j].flags << std::dec << std::endl;
				
				out << "Normals: " << l->normals.size() << std::endl;
				
				for (int j=0; j<l->normals.size() && options & OPTION_INFO_FULL; j++)
					out << "\t" << j << " - x:" << l->normals[j].X() << " y:" << l->normals[j].Y() << " z:" << l->normals[j].Z() << std::endl;
				
				out << "Faces: " << l->faces.size() << std::endl;
				
				for (int j=0; j<l->faces.size() && options & OPTION_INFO_FULL; j++)
					out << "\t" << j << " - texture:" << l->faces[j].texture << " type:" << l->faces[j].type << " flags:0x" << std::hex << l->faces[j].flags << std::dec << std::endl;
				
				out << "Tags: " << l->tags.size() << std::endl;

				for (int j=0; j<l->tags.size(); j++) {
					if (l->tags[j] == "#SharpEdges#") {
						out << "\t" << l->tags[j] << " count: " << l->sharp_edges.size()/2;
						
						for (int k=0; k<l->sharp_edges.size() && options & OPTION_INFO_FULL; k++)
							if (k%2)
								out << " " << l->sharp_edges[k];
							else
								out << std::endl << "\t\t" << k/2 << " - " << l->sharp_edges[k];
							
						out << std::endl;
					}
					else
					if (l->tags[j] == "#Property#") {
						if (l->properties.size()>1  &&  !property_done) {
							property_done = true;
							out << "\t" << l->tags[j] << std::endl;
							
							for (int j=0; j<l->properties.size()/2; j+=2)
								out << "\t\t" << l->properties[j] << "=" << l->properties[j+1] << std::endl;
						}
					}
					else
					if (l->tags[j] == "#Animation#") {
						if (l->animations.size()>0  &&  !animation_done) {
							animation_done = true;
							out << "\t" << l->tags[j] << " frames: " << l->animations.size() << std::endl;
							
							if (options & OPTION_INFO_FULL) {
								for (int animation_index=0; animation_index<l->animations.size(); animation_index++) {
									out << "\t\t" << animation_index << " - Time: " << l->animations[animation_index].frame_time << std::endl;
									
									for (int bone_index=0; bone_index<l->animations[animation_index].bone_pos.size(); bone_index++)
										out << "\t\t\t" << 
										" x:" << l->animations[animation_index].bone_pos[bone_index].X() << 
										" y:" << l->animations[animation_index].bone_pos[bone_index].Y() << 
										" z:" << l->animations[animation_index].bone_pos[bone_index].Z() << 
										std::endl;
								}
							}
						}
					}
					else
					if (l->tags[j] == "#Mass#") {
						double mass_total = 0;
						std::string list  = "";
						
						for (int k=0; k<l->mass.size(); k++) {
							if (options & OPTION_INFO_FULL)
								list += "\n\t\t" + std::to_string(k) + " - " + std::to_string(l->mass[k]);
							
							mass_total += l->mass[k];
						}
						
						out << "\t" << l->tags[j] << " sum:" << mass_total;
						
						if (options & OPTION_INFO_FULL)
							out << list;
						
						out << std::endl;
					}
					else
					if (l->tags[j] == "#MaterialIndex#") {
						out << "\t" << l->tags[j] << std::endl
						<< "Ambient r:" << (int)l->materials[i].ambient.r << " g:" << (int)l->materials[i].ambient.g << " b:" << (int)l->materials[i].ambient.b << " a:" << (int)l->materials[i].ambient.a << std::endl 
						<< "Diffuse r:" << (int)l->materials[i].diffuse.r << " g:" << (int)l->materials[i].diffuse.g << " b:" << (int)l->materials[i].diffuse.b << " a:" << (int)l->materials[i].diffuse.a << std::endl 
						<< "Specular r:" << (int)l->materials[i].specular.r << " g:" << (int)l->materials[i].specular.g << " b:" << (int)l->materials[i].specular.b << " a:" << (int)l->materials[i].specular.a << std::endl 
						<< "Emissive r:" << (int)l->materials[i].emissive.r << " g:" << (int)l->materials[i].emissive.g << " b:" << (int)l->materials[i].emissive.b << " a:" << (int)l->materials[i].emissive.a << std::endl;
					}
					else {
						out << "\t" << l->tags[j];
						
						// Byte listing produces too large file
						/*if (l->tags[j][0] != '#') {
							if (options & OPTION_INFO_FULL  &&  l->tags[j][0]!='-'  &&  l->tags[j][0]!='.') {
								out << " " << l->named_selections[nselection_index].point.size();
								
								for (int k=0; k<l->named_selections[nselection_index].point.size(); k++) {
									out << std::endl 
									<< "\t\t" << k << " - " 
									<< std::to_string(l->named_selections[nselection_index].point[k]) << " ";
								}
								
								for (int k=0; k<k<l->named_selections[nselection_index].face.size(); k++) {
									out << std::endl 
									<< "\t\t" << k << " - " 
									<< std::to_string(l->named_selections[nselection_index].face[k]);
								}
							}
							
							nselection_index++;
						}*/
						
						out << std::endl;
					}
				}
			}
		} 
		else {
			#ifdef SYSTEM_WINDOWS
				filename_output = CreateOutPath(filename_input);
				CopyFile(filename_input.c_str(), filename_output.c_str(), 0);
				global.files_to_skip.push_back(filename_output);
			#endif
		}
	}

	if (out.is_open())
		out.close();

	return 0;
}

#ifdef SYSTEM_WINDOWS
	std::string Int2Str(int num, bool leading_zero=false)
	{
		std::ostringstream text;
		
		if (leading_zero  &&  num<10)
			text << "0";
		
		text << num;
		return text.str();
	}

	std::string FormatFileInfo(FILETIME const& ft, DWORD bytes)
	{
		SYSTEMTIME st;
		FileTimeToSystemTime(&ft, &st);
		
		std::string output = 
			Int2Str(st.wYear) + "." + 
			Int2Str(st.wMonth,1) + "." + 
			Int2Str(st.wDay,1) + " " + 
			Int2Str(st.wHour,1) + ":" + 
			Int2Str(st.wMinute,1) + ":" + 
			Int2Str(st.wSecond,1) + " - " +
			Int2Str(bytes) + " bytes (";
		
		double size[]      = {(double)bytes, 0, 0};
		std::string name[] = {"B", "KB", "MB"};
		
		enum SIZE_NAMES {
			BYTES,
			KILOBYTES,
			MEGABYTES
		};
	
		if (size[BYTES] >= 1048576) {
			size[MEGABYTES]  = size[BYTES] / 1048576;
			size[MEGABYTES] -= fmod(size[MEGABYTES], 1);
			size[BYTES]     -= size[MEGABYTES] * 1048576;
		}
	
		if (size[BYTES] >= 1024) {
			size[KILOBYTES]  = size[BYTES] / 1024;
			size[KILOBYTES] -= fmod(size[KILOBYTES], 1);
			size[BYTES]     -= size[KILOBYTES] * 1024;
		}
		
		int select = 2;
		
		while (select >= 0)
			if (size[select] == 0)
				select--;
			else
				break;
	
		double final_size = size[select];
		
		if (select > 0)
			final_size += size[select-1] / 1024;
			
		std::ostringstream tempstr;
		tempstr << std::fixed << std::setprecision(select==2 ? 2 : 0) << final_size;
		output += tempstr.str() + " " + name[select] + ")";
		return output;
	}

	void FormatError(DWORD error)
	{
		if (error == 0) 
			return;

		LPTSTR errorText = NULL;

		FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error, 
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&errorText,
		0,
		NULL);

		std::cout <<  "   - " << (char*)errorText << std::endl;

		if (errorText != NULL)
			LocalFree(errorText);
	}
	
	int ScanDirectory(std::string path, int &options)
	{
		WIN32_FIND_DATA fd;
		std::string wildcard = path + "\\*";
		HANDLE hFind         = FindFirstFile(wildcard.c_str(), &fd);
		int return_value     = 0;

		if (hFind == INVALID_HANDLE_VALUE) {
			FormatError(GetLastError());
			return 2;
		}
		
		do {
			std::string file_name = (std::string)fd.cFileName;
			std::string full_path = path + "\\" + file_name;
			
			if (file_name == "." || file_name == "..")
				continue;
				
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				if (options & OPTION_RECURSIVE)
					ScanDirectory(full_path, options);
			} else {
				size_t dot = file_name.find_last_of('.');
				
				if (dot != std::string::npos) {
					std::string extension = file_name.substr(dot + 1);
					transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
					
					if (extension == "p3d") {
						bool file_ok = true;
						
						// Ignore files that were created by this program in this instance
						for (int i=0; i<global.files_to_skip.size() && file_ok; i++)
							if (full_path == global.files_to_skip[i])
								file_ok = false;
						
						if (file_ok) {
							global.files_total++;
							Parse_P3D(full_path, FormatFileInfo(fd.ftLastWriteTime, fd.nFileSizeLow), options);
						}
					}
				}
			}
		} while (FindNextFile(hFind, &fd) != 0);
		
		FindClose(hFind);
		return 0;
	}
#endif

int main(int argc, char* argv[]) {
	int return_value = 0;
	
	if (argc < 2) {
		std::cout << 
		"odol2mlod v1.0 by Miki and Faguss (ofp-faguss.com)" << std::endl <<
		"Converts OFP/CWA P3D model from the ODOL format to the MLOD format" << std::endl << std::endl <<
		"Usage: odol2mlod [options] <file or dir> ..." << std::endl << std::endl <<
		"Options:" << std::endl << 
		"\t-m merge vertices (instead of splitting them)" << std::endl << 
		"\t-M merge vertices only for the functional lods and not the graphical ones" << std::endl << 
		"\t-u use only user value for vertex lighting (instead of lighting selection)" << std::endl << 
		"\t-r scan directories recursively" << std::endl <<
		"\t-i create info file (instead of converting)" << std::endl <<
		"\t-I create full info file (instead of converting)" << std::endl <<
		"\t-s create a single info file (instead of one for each model)" << std::endl;
		return_value = 1;
	} else {
		int options = OPTION_NONE;
		
		for (int i=1; i<argc; i++) {
			if (argv[i][0] == '-') {
				for (int j=1; argv[i][j]!='\0'; j++) {
					switch(argv[i][j]) {
						case 'I' : options |= OPTION_INFO | OPTION_INFO_FULL; break;
						case 'i' : options |= OPTION_INFO; break;
						case 'r' : options |= OPTION_RECURSIVE; break;
						case 's' : options |= OPTION_SINGLELOG | OPTION_TRUNCATE; break;
						case 'm' : options |= OPTION_MERGE_POINTS; break;
						case 'M' : options |= OPTION_MERGE_POINTS_SELECTIVE; break;
						case 'u' : options |= OPTION_ONLY_USER_VALUE; break;
					}
				}
			} else {
				#ifdef SYSTEM_WINDOWS
					WIN32_FILE_ATTRIBUTE_DATA fd;
					if (GetFileAttributesEx(argv[i], GetFileExInfoStandard, &fd)) {
						if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
							if (ScanDirectory(argv[i], options)) {
								if (global.files_ok > 0)
									return_value = 0;
								else
									return_value = 3;
							} else
								return_value = 2;
						} else
							if (Parse_P3D(argv[i], FormatFileInfo(fd.ftLastWriteTime, fd.nFileSizeLow), options) == 0)
								return_value = 0;
							else
								return_value = 3;
					} else {
						FormatError(GetLastError());
						return_value = 2;
					}
				#else
					Parse_P3D(argv[i], "", options);
				#endif
			}
		}
	}

	if (global.files_total > 1)
		std::cout << "Files ok: " << global.files_ok << "/" << global.files_total << std::endl;
					
	return return_value;
}