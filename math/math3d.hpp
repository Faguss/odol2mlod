#pragma once
#include <cmath>
#include <array>

template<class Type>
class UnitQuaternionBase;

template <class T>
class Vector3Base
{
	template <class T2>
	friend class Vector3Base;
public:

	constexpr Vector3Base() = default;
	constexpr Vector3Base(const Vector3Base& other) = default;
	constexpr Vector3Base& operator=(const Vector3Base& other) = default;
	constexpr Vector3Base(Vector3Base& other) = default;
	constexpr Vector3Base& operator=(Vector3Base& other) = default;

	template<class U>
	constexpr Vector3Base(const Vector3Base<U>& other) noexcept:
		m_x(static_cast<T>(other.m_x)),
		m_y(static_cast<T>(other.m_y)),
		m_z(static_cast<T>(other.m_z))
	{}

	template<class U>
	constexpr Vector3Base& operator=(const Vector3Base<U>& other) noexcept
	{
		m_x = static_cast<T>(other.m_x);
		m_y = static_cast<T>(other.m_y);
		m_z = static_cast<T>(other.m_z);
		return *this;
	}

	constexpr Vector3Base(
		T x,
		T y,
		T z) noexcept:

		m_x(x),
		m_y(y),
		m_z(z)
	{}


	constexpr Vector3Base(T value) noexcept:
		m_x(value),
		m_y(value),
		m_z(value)
	{}

	constexpr T X() const noexcept {return m_x; }
	constexpr T& X() noexcept {return m_x; }
	constexpr T Y() const noexcept {return m_y; }
	constexpr T& Y() noexcept {return m_y; }
	constexpr T Z() const {return m_z; }
	constexpr T& Z() noexcept {return m_z; }
	
	

	constexpr T Dot(const Vector3Base& other) const noexcept
	{
		return  ( m_x * other.X()
				+ m_y * other.Y()
				+ m_z * other.Z());
	}

	constexpr Vector3Base Cross(const Vector3Base& other) const noexcept
	{
		return Vector3Base(
			m_y * other.Z() - m_z * other.Y(),
			m_z * other.X() - m_x * other.Z(),
			m_x * other.Y() - m_y * other.X());
	};


	constexpr T Size() const noexcept {return std::sqrt(Size2()); }
	constexpr T Size2() const noexcept {return Dot(*this); }


	void Normalize() noexcept
	{
		const T invSize = static_cast<T>(1.0) / Size();

		m_x *= invSize;
		m_y *= invSize;
		m_z *= invSize;
	}

	constexpr Vector3Base ReturnNormalized() const noexcept {return *this * (static_cast<T>(1.0) / Size()); }

	constexpr T Distance2(const Vector3Base& other) const noexcept
	{
		const auto diff = *this - other;
		return diff.Dot(diff);
	}

	constexpr T Distance(const Vector3Base& other) const noexcept {return std::sqrt(Distance2(other));}

	constexpr Vector3Base Interpolate(const Vector3Base& b, T coef) const noexcept
	{
		return *this * (static_cast<T>(1.0) - coef) + b * coef;
	}
	
	constexpr Vector3Base operator - () const noexcept {return Vector3Base(-m_x, -m_y, -m_z); }


	constexpr Vector3Base operator + (const Vector3Base& other) const noexcept
	{
		return Vector3Base(
			m_x + other.X(),
			m_y + other.Y(),
			m_z + other.Z());
	}

	constexpr Vector3Base operator - (const Vector3Base& other) const noexcept
	{
		return Vector3Base(
			m_x - other.X(),
			m_y - other.Y(),
			m_z - other.Z());
	}

	
	constexpr Vector3Base operator * (T scaler) const noexcept
	{
		return Vector3Base(
			m_x * scaler,
			m_y * scaler,
			m_z * scaler);
	}

	
	constexpr Vector3Base operator / (T scaler) const noexcept
	{
		return Vector3Base(
			m_x / scaler,
			m_y / scaler,
			m_z / scaler);
	}

	constexpr Vector3Base operator * (const Vector3Base& other) const noexcept
	{
		return Vector3Base(
			m_x * other.m_x,
			m_y * other.m_y,
			m_z * other.m_z);
	}

	
	constexpr Vector3Base operator / (const Vector3Base& other) const noexcept
	{
		return Vector3Base(
			m_x / other.m_x,
			m_y / other.m_y,
			m_z / other.m_z);
	}

	constexpr void operator += (const Vector3Base& other) noexcept
	{
		m_x += other.X();
		m_y += other.Y();
		m_z += other.Z();
	}


	constexpr void operator -= (const Vector3Base& other) noexcept
	{
		m_x -= other.X();
		m_y -= other.Y();
		m_z -= other.Z();
	}

	constexpr void operator *= (T scaler) noexcept
	{
		m_x *= scaler;
		m_y *= scaler;
		m_z *= scaler;
	}


	constexpr void operator /= (T scaler) noexcept
	{
		m_x /= scaler;
		m_y /= scaler;
		m_z /= scaler;
	}


	constexpr void operator *= (const Vector3Base& other) noexcept
	{
		m_x *= other.m_x;
		m_y *= other.m_y;
		m_z *= other.m_z;
	}


	constexpr void operator /= (const Vector3Base& other) noexcept
	{
		m_x /= other.m_x;
		m_y /= other.m_y;
		m_z /= other.m_z;
	}


	constexpr Vector3Base Abs() const noexcept
	{
		return Vector3Base(
			std::abs(m_x),
			std::abs(m_y),
			std::abs(m_z));
	}


	constexpr T Max() const noexcept {return std::max(m_x, std::max(m_y, m_z)); }

	constexpr T Min() const noexcept {return std::min(m_x, std::min(m_y, m_z)); }


	constexpr void StoreMin(const Vector3Base& other) noexcept
	{
		m_x = std::min(m_x, other.X());
		m_y = std::min(m_y, other.Y());
		m_z = std::min(m_z, other.Z());
	}


	constexpr void StoreMax(const Vector3Base& other) noexcept
	{
		m_x = std::max(m_x, other.X());
		m_y = std::max(m_y, other.Y());
		m_z = std::max(m_z, other.Z());
	}

private:
	T m_x;
	T m_y;
	T m_z;
};


template <class T>
struct Vector3Constant
{
	static constexpr Vector3Base<T> Zero = Vector3Base<T>(0.0, 0.0, 0.0);
	static constexpr Vector3Base<T> Aside = Vector3Base<T>(1.0, 0.0, 0.0);
	static constexpr Vector3Base<T> Up = Vector3Base<T>(0.0, 1.0, 0.0);
	static constexpr Vector3Base<T> Direction = Vector3Base<T>(0.0 , 0.0, 1.0);
};

template <class T>
struct  Matrix3Constant;

// always orthonormal
template <class T>
class Matrix3Base
{
template<class Type>
friend class UnitQuaternionBase;

friend struct Matrix3Constant<T>;
public:
	constexpr Matrix3Base() = default; // not valid object, but woud be hard to use in containers othewise

	constexpr Matrix3Base(const Matrix3Base& other) = default;
	constexpr Matrix3Base& operator = (const Matrix3Base& other) = default;
	constexpr Matrix3Base(Matrix3Base& other) = default;
	constexpr Matrix3Base& operator = (Matrix3Base& other) = default;

	constexpr Matrix3Base(
		const Vector3Base<T>& dir, 
		const Vector3Base<T>& up) noexcept:

		m_aside(dir.Cross(-up).ReturnNormalized()),
		m_dir(dir.ReturnNormalized())
	{
		// cant beset in initializer lsit cos of initializatin order
		m_up = m_dir.Cross(m_aside);
	}

	constexpr std::array<float, 9> GetOpenGLMat3() const noexcept
	{
		return {
			static_cast<float>(m_aside.X()),
			static_cast<float>(m_aside.Y()),
			static_cast<float>(m_aside.Z()),

			static_cast<float>(m_up.X()),
			static_cast<float>(m_up.Y()),
			static_cast<float>(m_up.Z()),

			static_cast<float>(m_dir.X()),
			static_cast<float>(m_dir.Y()),
			static_cast<float>(m_dir.Z())};
	}


	constexpr std::array<float, 12> GetOpenGLMat3x4() const noexcept
	{
		return {
			static_cast<float>(m_aside.X()),
			static_cast<float>(m_aside.Y()),
			static_cast<float>(m_aside.Z()),
			0.0f,

			static_cast<float>(m_up.X()),
			static_cast<float>(m_up.Y()),
			static_cast<float>(m_up.Z()),
			0.0f,

			static_cast<float>(m_dir.X()),
			static_cast<float>(m_dir.Y()),
			static_cast<float>(m_dir.Z()),
			0.0f};
	}


	constexpr void SetDirectionAndUp(
		const Vector3Base<T>& dir, 
		const Vector3Base<T>& up) noexcept
	{
		m_dir = dir.ReturnNormalized();
		m_aside = dir.Cross(-up).ReturnNormalized();
		m_up = m_dir.Cross(m_aside);
	}

	constexpr void SetUpAndDirection( 
		const Vector3Base<T>& up,
		const Vector3Base<T>& dir) noexcept
	{
		m_up = up.ReturnNormalized();
		m_aside = dir.Cross(-up).ReturnNormalized();
		m_dir = m_up.Cross(m_aside);
	}



	constexpr const Vector3Base<T>& Direction() const noexcept{return m_dir; }

	constexpr const Vector3Base<T>& Up() const noexcept {return m_up; }

	constexpr const Vector3Base<T>& Aside() const noexcept {return m_aside; }


	inline constexpr void SetIdentity() noexcept;

	constexpr void SetRotationX(T angle) noexcept
	{
		SetIdentity();
		const T s = std::sin(angle);
		const T c = std::cos(angle);

		m_up.Y() = c;
		m_up.Z() = s;
		m_dir.Y() = -s;
		m_dir.Z() = c;	
	}


	constexpr void SetRotationY(T angle) noexcept
	{
		SetIdentity();
		const T s = std::sin(angle);
		const T c = std::cos(angle);

		m_aside.X() = c;
		m_aside.Z() = s;

		m_dir.X() = -s;
		m_dir.Z() = c;
	}


	constexpr void SetRotationZ(T angle) noexcept
	{
		SetIdentity();
		const T s = std::sin(angle);
		const T c = std::cos(angle);

		m_aside.X() = c;
		m_aside.Y() = s;

		m_up.X() = -s;
		m_up.Y() = c;
	}


	constexpr Vector3Base<T> operator * (const Vector3Base<T>& vector) const noexcept
	{
		return Vector3Base<T>(m_aside * vector.X() + m_up * vector.Y() + m_dir * vector.Z());
	}


	constexpr Matrix3Base operator * (const Matrix3Base& other) const noexcept
	{
		return Matrix3Base(
			  m_aside * other.m_aside.X()
			+ m_up    * other.m_aside.Y()
			+ m_dir   * other.m_aside.Z(),

			  m_aside * other.m_up.X()
			+ m_up    * other.m_up.Y()
			+ m_dir   * other.m_up.Z(),

			  m_aside * other.m_dir.X()
			+ m_up    * other.m_dir.Y()
			+ m_dir   * other.m_dir.Z());
	}


	constexpr void Transpose() noexcept
	{
		std::swap(m_aside.Y(), m_up.X());
		std::swap(m_aside.Z(), m_dir.X());
		std::swap(m_up.Z(), m_dir.Y());
	}

	constexpr Matrix3Base ReturnTransposed() const noexcept
	{
		return Matrix3Base(
			Vector3Base<T>(m_aside.X(), m_up.X(), m_dir.X()),
			Vector3Base<T>(m_aside.Y(), m_up.Y(), m_dir.Y()),
			Vector3Base<T>(m_aside.Z(), m_up.Z(), m_dir.Z()));
	}

	constexpr Matrix3Base Interpolate(const Matrix3Base& b, T coef) const noexcept
	{
		return Matrix3Base(m_dir.Interpolate(b.m_dir, coef),
			m_up.Interpolate(b.m_up, coef));
	}

private:
	constexpr Matrix3Base(
		const Vector3Base<T>& aside,
		const Vector3Base<T>& up,
		const Vector3Base<T>& dir) noexcept:

		m_aside(aside),
		m_up(up),
		m_dir(dir)
	{}

	Vector3Base<T> m_aside;
	Vector3Base<T> m_up;
	Vector3Base<T> m_dir;
};

template <class T>
struct  Matrix3Constant
{
	static constexpr Matrix3Base<T> Identity = Matrix3Base<T>(Vector3Constant<T>::Aside, Vector3Constant<T>::Up, Vector3Constant<T>::Direction);
};

template <class T>
constexpr void Matrix3Base<T>::SetIdentity() noexcept
{
	*this = Matrix3Constant<T>::Identity;
}

template <class T>
struct  Matrix3ScaledConstant;

// always orthogonal
template <class T>
class Matrix3BaseScaled
{
friend struct Matrix3ScaledConstant<T>;
public:
	constexpr Matrix3BaseScaled() = default; // not valid object, but woud be hard to use in containers othewise

	constexpr Matrix3BaseScaled(const Matrix3BaseScaled& other) = default;
	constexpr Matrix3BaseScaled& operator = (const Matrix3BaseScaled& other) = default;
	constexpr Matrix3BaseScaled(Matrix3BaseScaled& other) = default;
	constexpr Matrix3BaseScaled& operator = (Matrix3BaseScaled& other) = default;

	constexpr Matrix3BaseScaled (
		const Vector3Base<T>& dir, 
		const Vector3Base<T>& up) noexcept:

		m_aside(dir.Cross(-up).ReturnNormalized()),
		m_dir(dir.ReturnNormalized())
	{
		// cant beset in initializer lsit cos of initializatin order
		m_up = m_dir.Cross(m_aside);
	}

	constexpr Matrix3BaseScaled(const Matrix3Base<T>& other) noexcept:
		m_aside(other.Aside()),
		m_up(other.Up()),
		m_dir(other.Direction())
	{}

	constexpr Matrix3BaseScaled(
		const Matrix3Base<T>& other,
		const Vector3Base<T>& scales) noexcept :
		m_aside(other.Aside()),
		m_up(other.Up()),
		m_dir(other.Direction())
	{
		SetScales(scales);
	}

	constexpr void SetScales(const Vector3Base<T>& scales) noexcept
	{
		m_aside *= scales.X();
		m_up *= scales.Y();
		m_dir *= scales.Z();
	}

	constexpr void SetScaleAside(T scale) noexcept {m_aside *= scale; }
	constexpr void SetScaleUp(T scale) noexcept {m_up *= scale; }
	constexpr void SetScaleDirection(T scale) noexcept {m_dir *= scale; }

	constexpr std::array<float, 9> GetOpenGLMat3() const noexcept
	{
		return {
			static_cast<float>(m_aside.X()),
			static_cast<float>(m_aside.Y()),
			static_cast<float>(m_aside.Z()),

			static_cast<float>(m_up.X()),
			static_cast<float>(m_up.Y()),
			static_cast<float>(m_up.Z()),

			static_cast<float>(m_dir.X()),
			static_cast<float>(m_dir.Y()),
			static_cast<float>(m_dir.Z())};
	}


	constexpr std::array<float, 12> GetOpenGLMat3x4() const noexcept
	{
		return {
			static_cast<float>(m_aside.X()),
			static_cast<float>(m_aside.Y()),
			static_cast<float>(m_aside.Z()),
			0.0f,

			static_cast<float>(m_up.X()),
			static_cast<float>(m_up.Y()),
			static_cast<float>(m_up.Z()),
			0.0f,

			static_cast<float>(m_dir.X()),
			static_cast<float>(m_dir.Y()),
			static_cast<float>(m_dir.Z()),
			0.0f};
	}


	constexpr void SetDirectionAndUp(
		const Vector3Base<T>& dir, 
		const Vector3Base<T>& up) noexcept
	{
		m_dir = dir.ReturnNormalized();
		m_aside = dir.Cross(-up).ReturnNormalized();
		m_up = m_dir.Cross(m_aside);
	}

	constexpr void SetUpAndDirection( 
		const Vector3Base<T>& up,
		const Vector3Base<T>& dir) noexcept
	{
		m_up = up.ReturnNormalized();
		m_aside = dir.Cross(-up).ReturnNormalized();
		m_dir = m_up.Cross(m_aside);
	}


	constexpr const Vector3Base<T>& Direction() const noexcept {return m_dir; }
	constexpr const Vector3Base<T>& Up() const noexcept {return m_up; }
	constexpr const Vector3Base<T>& Aside() const noexcept {return m_aside; }

	inline constexpr void SetIdentity() noexcept;

	constexpr void SetRotationX(T angle) noexcept
	{
		SetIdentity();
		const T s = std::sin(angle);
		const T c = std::cos(angle);

		m_up.Y() = c;
		m_up.Z() = s;
		m_dir.Y() = -s;
		m_dir.Z() = c;	
	}


	constexpr void SetRotationY(T angle) noexcept
	{
		SetIdentity();
		const T s = std::sin(angle);
		const T c = std::cos(angle);

		m_aside.X() = c;
		m_aside.Z() = s;

		m_dir.X() = -s;
		m_dir.Z() = c;
	}


	constexpr void SetRotationZ(T angle) noexcept
	{
		SetIdentity();
		const T s = std::sin(angle);
		const T c = std::cos(angle);

		m_aside.X() = c;
		m_aside.Y() = s;

		m_up.X() = -s;
		m_up.Y() = c;
	}


	constexpr Vector3Base<T> operator*(const Vector3Base<T>& vector) const noexcept
	{
		return Vector3Base<T>(m_aside * vector.X() + m_up * vector.Y() + m_dir * vector.Z());
	}


	constexpr Matrix3BaseScaled operator*(const Matrix3BaseScaled& other) const noexcept
	{
		return Matrix3BaseScaled(
			  m_aside * other.m_aside.X()
			+ m_up    * other.m_aside.Y()
			+ m_dir   * other.m_aside.Z(),

			  m_aside * other.m_up.X()
			+ m_up    * other.m_up.Y()
			+ m_dir   * other.m_up.Z(),

			  m_aside * other.m_dir.X()
			+ m_up    * other.m_dir.Y()
			+ m_dir   * other.m_dir.Z());
	}


	constexpr Matrix3BaseScaled operator*(const Matrix3Base<T>& other) const noexcept
	{
		return Matrix3BaseScaled(
			  m_aside * other.Aside().X()
			+ m_up    * other.Aside().Y()
			+ m_dir   * other.Aside().Z(),

			  m_aside * other.Up().X()
			+ m_up    * other.Up().Y()
			+ m_dir   * other.Up().Z(),

			  m_aside * other.Direction().X()
			+ m_up    * other.Direction().Y()
			+ m_dir   * other.Direction().Z());
	}


	constexpr Matrix3BaseScaled MultiplyLeft(const Matrix3Base<T>& lhs) const noexcept {
		return Matrix3BaseScaled(
			  lhs.Aside() * m_aside.X()
			+ lhs.Up()    * m_aside.Y()
			+ lhs.Direction()   * m_aside.Z(),

			  lhs.Aside() * m_up.X()
			+ lhs.Up()    * m_up.Y()
			+ lhs.Direction()   * m_up.Z(),

			  lhs.Aside() * m_dir.X()
			+ lhs.Up()    * m_dir.Y()
			+ lhs.Direction()   * m_dir.Z());
	}

	constexpr void Inverse() noexcept
	{
		const Vector3Base<T> invScales(
			static_cast<T>(1.0) / m_aside.Size2(),
			static_cast<T>(1.0) / m_up.Size2(),
			static_cast<T>(1.0) / m_dir.Size2());

		std::swap(m_aside.Y(), m_up.X());
		std::swap(m_aside.Z(), m_dir.X());
		std::swap(m_up.Z(), m_dir.Y());

		m_aside *= invScales;
		m_up *= invScales;
		m_dir *= invScales;
	}

	constexpr Matrix3BaseScaled ReturnInverse() const noexcept
	{
		const Vector3Base<T> invScales(
			static_cast<T>(1.0) / m_aside.Size2(),
			static_cast<T>(1.0) / m_up.Size2(),
			static_cast<T>(1.0) / m_dir.Size2());

		return Matrix3BaseScaled(
			Vector3Base<T>(m_aside.X(), m_up.X(), m_dir.X()) * invScales,
			Vector3Base<T>(m_aside.Y(), m_up.Y(), m_dir.Y())  * invScales,
			Vector3Base<T>(m_aside.Z(), m_up.Z(), m_dir.Z())  * invScales);
	}


private:
	constexpr Matrix3BaseScaled(
		const Vector3Base<T>& aside,
		const Vector3Base<T>& up,
		const Vector3Base<T>& dir) noexcept:

		m_aside(aside),
		m_up(up),
		m_dir(dir)
	{}

	Vector3Base<T> m_aside;
	Vector3Base<T> m_up;
	Vector3Base<T> m_dir;
};

template <class T>
struct  Matrix3ScaledConstant
{
	static constexpr Matrix3BaseScaled<T> Identity = Matrix3BaseScaled<T>(Vector3Constant<T>::Aside, Vector3Constant<T>::Up, Vector3Constant<T>::Direction);
};


template <class T>
inline constexpr Matrix3BaseScaled<T> operator*(
	const Matrix3Base<T>& lhs,
	const Matrix3BaseScaled<T>& rhs) noexcept
{
	return rhs.MultiplyLeft(lhs);
}

template <class T>
constexpr void Matrix3BaseScaled<T>::SetIdentity() noexcept
{
	*this = Matrix3ScaledConstant<T>::Identity;
}

// always orthonormal
template<class T>
class Matrix4Base
{
public:
	constexpr Matrix4Base() = default; // not valid object, but woud be hard to use in containers othewise
	constexpr Matrix4Base(const Matrix4Base& other) = default;
	constexpr Matrix4Base& operator = (const Matrix4Base& other) = default;
	constexpr Matrix4Base(Matrix4Base& other) = default;
	constexpr Matrix4Base& operator = (Matrix4Base& other) = default;


	
	constexpr Matrix4Base(
		const Matrix3Base<T>& orientation,
		const Vector3Base<T>& position) noexcept :
		
		m_orientation(orientation),
		m_position(position)
	{}
	
	constexpr Matrix4Base(const Vector3Base<T>& dir, 
		const Vector3Base<T>& up,
		const Vector3Base<T>& position) noexcept :

		m_orientation(dir, up),
		m_position(position)
	{}

	constexpr std::array<float, 16> GetOpenGLMat4() const noexcept
	{
		return {
			static_cast<float>(m_orientation.Aside().X()),
			static_cast<float>(m_orientation.Aside().Y()),
			static_cast<float>(m_orientation.Aside().Z()),
			0.0f,

			static_cast<float>(m_orientation.Up().X()),
			static_cast<float>(m_orientation.Up().Y()),
			static_cast<float>(m_orientation.Up().Z()),
			0.0f,

			static_cast<float>(m_orientation.Direction().X()),
			static_cast<float>(m_orientation.Direction().Y()),
			static_cast<float>(m_orientation.Direction().Z()),
			0.0f,

			static_cast<float>(m_position.X()),
			static_cast<float>(m_position.Y()),
			static_cast<float>(m_position.Z()),
			1.0f};
	}


	constexpr std::array<float, 16> GetScaledOpenGLMat4(const Vector3Base<T>& scales) const noexcept
	{
		return {
			static_cast<float>(m_orientation.Aside().X() * scales.X()),
			static_cast<float>(m_orientation.Aside().Y() * scales.X()),
			static_cast<float>(m_orientation.Aside().Z() * scales.X()),
			0.0f,

			static_cast<float>(m_orientation.Up().X() * scales.Y()),
			static_cast<float>(m_orientation.Up().Y() * scales.Y()),
			static_cast<float>(m_orientation.Up().Z() * scales.Y()),
			0.0f,

			static_cast<float>(m_orientation.Direction().X() * scales.Z()),
			static_cast<float>(m_orientation.Direction().Y() * scales.Z()),
			static_cast<float>(m_orientation.Direction().Z() * scales.Z()),
			0.0f,

			static_cast<float>(m_position.X()),
			static_cast<float>(m_position.Y()),
			static_cast<float>(m_position.Z()),
			1.0f};
	}


	constexpr const Vector3Base<T>& Position() const noexcept {return m_position; } 

	constexpr Vector3Base<T>& Position() noexcept { return m_position; } 

	constexpr const Matrix3Base<T>& Orientation() const noexcept {return m_orientation; }

	constexpr Matrix3Base<T>& Orientation() noexcept {return m_orientation;}

	constexpr const Vector3Base<T>& Direction() const noexcept {return m_orientation.Direction();}

	constexpr const Vector3Base<T>& Up() const noexcept {return m_orientation.Up();}

	constexpr const Vector3Base<T>& Aside() const noexcept {return m_orientation.Aside();}





	inline constexpr void SetIdentity() noexcept;

	constexpr void SetRotationX(T angle) noexcept
	{
		m_orientation.SetRotationX(angle);
		m_position = Vector3Base<T>(0.0, 0.0, 0.0);
	}


	constexpr void SetRotationY(T angle) noexcept
	{
		m_orientation.SetRotationY(angle);
		m_position = Vector3Base<T>(0.0, 0.0, 0.0);
	}


	constexpr void SetRotationZ(T angle) noexcept
	{
		m_orientation.SetRotationZ(angle);
		m_position = Vector3Base<T>(0.0, 0.0, 0.0);
	}


	constexpr void SetDirectionAndUp(
		const Vector3Base<T>& dir, 
		const Vector3Base<T>& up) noexcept
	{
		m_orientation.SetDirectionAndUp(dir,up);
	}
	
	constexpr void SetUpAndDirection( 
		const Vector3Base<T>& up,
		const Vector3Base<T>& dir) noexcept
	{
		m_orientation.SetUpAndDirection(up, dir);
	}


	constexpr Vector3Base<T> operator * (const Vector3Base<T>& other) const noexcept
	{
		return m_orientation * other + m_position;
	}

	constexpr void MultiplyByTranslation(const Vector3Base<T>& position) noexcept
	{
		m_position = m_orientation * position + m_position;
	}

	constexpr Matrix4Base ReturnMultipliedByTranslation(const Vector3Base<T>& position) const noexcept
	{
		return Matrix4Base(m_orientation, m_orientation * position + m_position);
	}

	constexpr Matrix4Base operator * (const Matrix4Base& other) const noexcept
	{
		return Matrix4Base(m_orientation * other.m_orientation,
			m_orientation * other.m_position + m_position);
	}


	constexpr void Inverse() noexcept
	{
		m_orientation.Transpose();
		m_position = m_orientation * (-m_position);
	}

	constexpr Matrix4Base ReturnInverse() const noexcept
	{
		const auto transposed = m_orientation.ReturnTransposed();
		return Matrix4Base(transposed, transposed * (-m_position));
	}

	constexpr Matrix4Base Interpolate(const Matrix4Base& b, T coef) const noexcept
	{
		return Matrix4Base(m_orientation.Interpolate(b.m_orientation, coef),
			m_position.Interpolate(b.m_position, coef));
	}


private:

	Matrix3Base<T> m_orientation;
	Vector3Base<T> m_position;
};


template <class T>
struct  Matrix4Constant
{
	static constexpr Matrix4Base<T> Identity = Matrix4Base<T>(Matrix3Constant<T>::Identity, Vector3Constant<T>::Zero);
};

template <class T>
constexpr void Matrix4Base<T>::SetIdentity() noexcept
{
	*this = Matrix4Constant<T>::Identity;
}

// as of now mostly utility class
template<class T>
class Matrix4BaseScaled
{
public:	
	constexpr Matrix4BaseScaled() = default;
	constexpr Matrix4BaseScaled(const Matrix4BaseScaled& other) = default;
	constexpr Matrix4BaseScaled& operator = (const Matrix4BaseScaled& other) = default;
	constexpr Matrix4BaseScaled(Matrix4BaseScaled& other) = default;
	constexpr Matrix4BaseScaled& operator = (Matrix4BaseScaled& other) = default;


	constexpr Matrix4BaseScaled(
		const Matrix3BaseScaled<T>& orientation,
		const Vector3Base<T>& position) noexcept:
		
		m_orientation(orientation),
		m_position(position)
	{}

	constexpr Matrix4BaseScaled(
		const Matrix3BaseScaled<T>& orientation,
		const Vector3Base<T>& position,
		const Vector3Base<T>& scales) noexcept:
		
		m_orientation(orientation),
		m_position(position)
	{
		m_orientation.SetScales(scales);
	}


	constexpr Matrix4BaseScaled(
		const Matrix4Base<T>& matrix,
		const Vector3Base<T>& scales) noexcept:
	
		m_orientation(matrix.Orientation()),
		m_position(matrix.Position())
	{
		m_orientation.SetScales(scales);
	}

 	constexpr const Matrix3BaseScaled<T>& Orientation() const noexcept {return m_orientation; }
	constexpr Matrix3BaseScaled<T>& Orientation() noexcept {return m_orientation; }

 	constexpr const Vector3Base<T>& Position() const noexcept {return m_position; }
	constexpr Vector3Base<T>& Position() noexcept {return m_position; }


	constexpr std::array<float, 16> GetOpenGLMat4() const noexcept
	{
		return {
			static_cast<float>(m_orientation.Aside().X()),
			static_cast<float>(m_orientation.Aside().Y()),
			static_cast<float>(m_orientation.Aside().Z()),
			0.0f,

			static_cast<float>(m_orientation.Up().X()),
			static_cast<float>(m_orientation.Up().Y()),
			static_cast<float>(m_orientation.Up().Z()),
			0.0f,

			static_cast<float>(m_orientation.Direction().X()),
			static_cast<float>(m_orientation.Direction().Y()),
			static_cast<float>(m_orientation.Direction().Z()),
			0.0f,

			static_cast<float>(m_position.X()),
			static_cast<float>(m_position.Y()),
			static_cast<float>(m_position.Z()),
			1.0f};
	}



	constexpr Vector3Base<T> operator * (const Vector3Base<T>& other) const  noexcept
	{
		return m_orientation * other + m_position;
	}


	constexpr void MultiplyByTranslation(const Vector3Base<T>& position) noexcept
	{
		m_position = m_orientation * position + m_position;
	}


	constexpr Matrix4BaseScaled ReturnMultipliedByTranslation(const Vector3Base<T>& position) const noexcept
	{
		return Matrix4BaseScaled(m_orientation, m_orientation * position + m_position);
	}

	constexpr Matrix4BaseScaled operator * (const Matrix4BaseScaled& other) const noexcept
	{
		return Matrix4BaseScaled(m_orientation * other.m_orientation,
			m_orientation * other.m_position + m_position);
	}

	constexpr Matrix4BaseScaled operator * (const Matrix4Base<T>& other) const noexcept
	{
		return Matrix4BaseScaled(m_orientation * other.Orientation(),
			m_orientation * other.Position() + m_position);
	}

	
	constexpr Matrix4BaseScaled MultiplyLeft(const Matrix4Base<T>& lhs) const noexcept
	{
		return Matrix4BaseScaled(lhs.Orientation() * m_orientation,
			lhs.Orientation() * m_position + lhs.Position());
	}

	constexpr void Inverse() noexcept
	{
		m_orientation.Inverse();
		m_position = m_orientation * -m_position;
	}

	constexpr Matrix4BaseScaled ReturnInverse() const noexcept
	{
		const auto inverseBasis = m_orientation.ReturnInverse();
		return Matrix4BaseScaled(inverseBasis, inverseBasis * -m_position);
	}

private:
	Matrix3BaseScaled<T> m_orientation;
	Vector3Base<T> m_position;
};

template <class T>
inline constexpr Matrix4BaseScaled<T> operator*(
	const Matrix4Base<T>& lhs,
	const Matrix4BaseScaled<T>& rhs) noexcept
{
	return rhs.MultiplyLeft(lhs);
}

class Matrix4x4
{
private:
	struct Vec4
	{
		constexpr Vec4() noexcept : f{0.0f, 0.0f, 0.0f, 0.0f}
		{}

		float f[4];
		constexpr inline const float &operator [] (int i) const noexcept
		{
			return f[i];
		}

		constexpr inline float &operator [] (int i) noexcept
		{
			return f[i];
		}

		constexpr void Negate() noexcept {
			f[0] =-f[0];
			f[1] =-f[1];
			f[2] =-f[2];
			f[3] =-f[3];
		}
	};
	Vec4 m_columns[4];

public:
	constexpr Matrix4x4() noexcept
	{}

	constexpr const Vec4& operator [] ( int i ) const noexcept
	{
		return m_columns[i];
	}

	constexpr Vec4& operator [] ( int i ) noexcept
	{
		return m_columns[i];
	}

	const float *GetOpenGLMat4() const noexcept
	{
		return reinterpret_cast<const float *>(this);
	}

	constexpr static Matrix4x4 Perspective(float clipNear, float clipFar, float invClipLeft, float invClipTop) noexcept
	{
		Matrix4x4 result;

		result.m_columns[0][0] = invClipLeft;
		result.m_columns[0][1] = 0.0f;
		result.m_columns[0][2] = 0.0f;
		result.m_columns[0][3] = 0.0f;

		result.m_columns[1][0] = 0.0f;
		result.m_columns[1][1] = invClipTop;
		result.m_columns[1][2] = 0.0f;
		result.m_columns[1][3] = 0.0f;

		result.m_columns[2][0] = 0.0f;
		result.m_columns[2][1] = 0.0f;
		result.m_columns[2][2] = (clipNear) / (clipNear - clipFar);
		result.m_columns[2][3] = 1.0f;

		result.m_columns[3][0] = 0.0f;
		result.m_columns[3][1] = 0.0f;
		result.m_columns[3][2] = - (clipFar * clipNear) / (clipNear - clipFar);
		result.m_columns[3][3] = 0.0f;

		return result;
	}

	constexpr static Matrix4x4 PerspectiveFov(float clipNear, float clipFar, float fovy, float aspect) noexcept
	{
		float tanHalfFovy = tan(fovy / 2.0f);

		Matrix4x4 result;
		result.m_columns[0][0] = 1.0f / (aspect * tanHalfFovy);
		result.m_columns[0][1] = 0.0f;
		result.m_columns[0][2] = 0.0f;
		result.m_columns[0][3] = 0.0f;

		result.m_columns[1][0] = 0.0f;
		result.m_columns[1][1] = 1.0f / (tanHalfFovy);
		result.m_columns[1][2] = 0.0f;
		result.m_columns[1][3] = 0.0f;

		result.m_columns[2][0] = 0.0f;
		result.m_columns[2][1] = 0.0f;
		result.m_columns[2][2] = (clipNear) / (clipNear - clipFar);
		result.m_columns[2][3] = 1.0f;

		result.m_columns[3][0] = 0.0f;
		result.m_columns[3][1] = 0.0f;
		result.m_columns[3][2] = - (clipFar * clipNear) / (clipNear - clipFar);
		result.m_columns[3][3] = 0.0f;

		return result;
	}

	constexpr static Matrix4x4 PerspectiveFov90(float clipNear, float clipFar) noexcept
	{
		Matrix4x4 result;
		result.m_columns[0][0] = 1.0f;
		result.m_columns[0][1] = 0.0f;
		result.m_columns[0][2] = 0.0f;
		result.m_columns[0][3] = 0.0f;

		result.m_columns[1][0] = 0.0f;
		result.m_columns[1][1] = 1.0f;
		result.m_columns[1][2] = 0.0f;
		result.m_columns[1][3] = 0.0f;

		result.m_columns[2][0] = 0.0f;
		result.m_columns[2][1] = 0.0f;
		result.m_columns[2][2] = (clipNear) / (clipNear - clipFar);
		result.m_columns[2][3] = 1.0f;

		result.m_columns[3][0] = 0.0f;
		result.m_columns[3][1] = 0.0f;
		result.m_columns[3][2] = - (clipFar * clipNear) / (clipNear - clipFar);
		result.m_columns[3][3] = 0.0f;

		return result;
	}

	constexpr static Matrix4x4 OrthoCentered(float width, float height, float length) noexcept
	{
		Matrix4x4 result;

		result.m_columns[0][0] = 2.0f / width;
		result.m_columns[0][1] = 0.0f;
		result.m_columns[0][2] = 0.0f;
		result.m_columns[0][3] = 0.0f;

		result.m_columns[1][0] = 0.0f;
		result.m_columns[1][1] = 2.0f / height;
		result.m_columns[1][2] = 0.0f;
		result.m_columns[1][3] = 0.0f;

		result.m_columns[2][0] = 0.0f;
		result.m_columns[2][1] = 0.0f;
		result.m_columns[2][2] = 1.0f / length;
		result.m_columns[2][3] = 0.0f;

		result.m_columns[3][0] = 0.0f;
		result.m_columns[3][1] = 0.0f;
		result.m_columns[3][2] = 0.5f;
		result.m_columns[3][3] = 1.0f;

		return result;
	}
};


using Vector3D = Vector3Base<double>;
using Matrix3D = Matrix3Base<double>;
using Matrix4D = Matrix4Base<double>;
using Matrix4DScaled = Matrix4BaseScaled<double>;

using Vector3F = Vector3Base<float>;
using Matrix3F = Matrix3Base<float>;
using Matrix4F = Matrix4Base<float>;
using Matrix4FScaled = Matrix4BaseScaled<float>;


using CVector3D = Vector3Constant<double>;
using CMatrix3D = Matrix3Constant<double>;
using CMatrix4D = Matrix4Constant<double>;

using CVector3F = Vector3Constant<float>;
using CMatrix3F = Matrix3Constant<float>;
using CMatrix4F = Matrix4Constant<float>;



