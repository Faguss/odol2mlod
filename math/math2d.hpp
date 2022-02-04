#pragma once
#include <cmath>

class Vector2
{
public:

	Vector2() = default;

	Vector2(
		const float x,
		const float y):

		m_x(x),
		m_y(y)
	{}

	float &X()
	{
		return m_x;
	}


	float &Y()
	{
		return m_y;
	}

	float X() const
	{
		return m_x;
	}


	float Y() const
	{
		return m_y;
	}

	// operators

	Vector2 operator + (const Vector2 other) const
	{
		return Vector2(m_x + other.m_x, m_y + other.m_y);		
	}

	Vector2 operator - (const Vector2 other) const
	{
		return Vector2(m_x - other.m_x, m_y - other.m_y);		
	}

	Vector2 operator * (const Vector2 other) const
	{
		return Vector2(m_x * other.m_x, m_y * other.m_y);		
	}

	Vector2 operator / (const Vector2 other) const
	{
		return Vector2(m_x / other.m_x, m_y / other.m_y);		
	}


	Vector2 operator * (const float other) const
	{
		return Vector2(m_x * other, m_y * other);		
	}

	Vector2 operator / (const float other) const
	{
		return Vector2(m_x / other, m_y / other);		
	}



	void operator += (const Vector2 other)
	{
		m_x += other.m_x;
		m_y += other.m_y;
	}

	void operator -= (const Vector2 other)
	{
		m_x -= other.m_x;
		m_y -= other.m_y;
	}

	void operator *= (const Vector2 other)
	{
		m_x *= other.m_x;
		m_y *= other.m_y;
	}

	void operator /= (const Vector2 other)
	{
		m_x /= other.m_x;
		m_y /= other.m_y;
	}

	void operator *= (const float other)
	{
		m_x *= other;
		m_y *= other;
	}

	void operator /= (const float other)
	{
		m_x /= other;
		m_y /= other;
	}



	float Dot(const Vector2 other) const
	{
		return m_x * other.m_x + m_y * other.m_y;
	}

	float Size2() const
	{
		return m_x * m_x + m_y * m_y; 
	}

	float Size() const
	{
		return std::sqrt(Size2());
	}


	void Normalize()
	{
		*this /= Size();
	}

	Vector2 ReturnPerpendicular() const
	{
		return Vector2(m_y, -m_x); 
	}


private:

	float m_x;
	float m_y;
};



inline bool IsInsideTriangle(
	Vector2 a,
	Vector2 b,
	Vector2 c,
	Vector2 point)
{

	const float det = (b.Y() - c.Y()) * (a.X() - c.X()) + (c.X() - b.X()) * (a.Y() - c.Y());
	
	const float l1 = ((b.Y() - c.Y()) * (point.X() - c.X()) + (c.X() - b.X()) * (point.Y() - c.Y())) / det;
	if (l1 <  0.0f || l1 > 1.0f)
	{
		return false;
	}

	const float l2 = ((c.Y() - a.Y()) * (point.X() - c.X()) + (a.X() - c.X()) * (point.Y() - c.Y())) / det;
	if (l2 <  0.0f || l2 > 1.0f)
	{
		return false;
	}

	return true;

	//const float l3 = 1.0f - l1 - l2;
}

