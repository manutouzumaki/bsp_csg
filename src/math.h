#ifndef _MATH_H_
#define _MATH_H_

#include <math.h>

#define VEC_EPSILON 0.000001f
#define MAT_EPSILON 0.000001f
#define PI 3.14159265359
#define TO_RAD(value) ((value/180.0f) * PI)
#define TO_DEG(value) ((value/PI) * 180.0f)

struct Vec2 
{
    union
    {
        struct
        {
            f32 x;
            f32 y;
        };
        f32 v[2];
    };

    f32 operator[](u32 index)
    {
        return v[index];
    }
};


Vec2 operator+(Vec2 a, Vec2 b)
{
    Vec2 result = {};
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}

Vec2 operator-(Vec2 a, Vec2 b)
{
    Vec2 result = {};
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}

Vec2 operator-(Vec2 v) {
    Vec2 result = {};
    result.x = -v.x;
    result.y = -v.y;
    return result;
}

Vec2 operator*(Vec2 a, Vec2 b) {
    Vec2 result = {};
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    return result;
}

Vec2 operator/(Vec2 a, Vec2 b) {
    Vec2 result = {};
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    return result;
}

Vec2 operator*(Vec2 v, f32 s) {
    Vec2 result = {};
    result.x = v.x * s;
    result.y = v.y * s;
    return result;
}

Vec2 operator/(Vec2 v, f32 s) {
    Vec2 result = {};
    result.x = v.x / s;
    result.y = v.y / s;
    return result;
}

f32 Vec2Dot(Vec2 a, Vec2 b) {
    f32 result = (a.x * b.x) + (a.y * b.y);
    return result;
}

f32 Vec2LenSq(Vec2 v) {
    f32 result = Vec2Dot(v, v);
    return result;

}

f32 Vec2Len(Vec2 v) {
    f32 result = Vec2LenSq(v);
    if(result < VEC_EPSILON) {
        return 0.0f;
    }
    return sqrtf(result);
}

void Vec2Normalize(Vec2 *v) {
    f32 lenSq = Vec2LenSq(*v);
    if(lenSq < VEC_EPSILON) {
        return;
    }
    f32 invLen = 1.0f / sqrtf(lenSq);
    v->x *= invLen;
    v->y *= invLen;
}

Vec2 Vec2Normalized(Vec2 v) {
    f32 lenSq = Vec2LenSq(v);
    if(lenSq < VEC_EPSILON) {
        return v;
    }
    f32 invLen = 1.0f / sqrtf(lenSq);
    Vec2 result{
        result.x = v.x * invLen,
        result.y = v.y * invLen,
    };
    return result;
}

f32 Vec2Angle(Vec2 a, Vec2 b) {
    f32 len = Vec2Len(a) * Vec2Len(b);
    if(len < VEC_EPSILON) {
        return 0.0f;
    }
    f32 dot = Vec2Dot(a, b);
    f32 result = acosf(dot / len);
    return result;
}

Vec2 Vec2Project(Vec2 a, Vec2 b) {
    f32 magB = Vec2Len(b);
    if(magB < VEC_EPSILON) {
        return {};
    }
    f32 scale = Vec2Dot(a, b) / magB;
    return b * scale;
}

Vec2 Vec2Reject(Vec2 a, Vec2 b) {
    Vec2 proj = Vec2Project(a, b);
    return a - proj;
}

Vec2 Vec2Reflect(Vec2 a, Vec2 b) {
    f32 magB = Vec2Len(b);
    if(magB < VEC_EPSILON) {
        return {};
    }
    f32 scale = Vec2Dot(a, b) / magB;
    Vec2 proj2 = b * (scale * 2.0f);
    return a - proj2;
}

Vec2 Vec2Lerp(Vec2 a, Vec2 b, f32 t) {
    return {
        (1 - t) * a.x + t * b.x,
        (1 - t) * a.y + t * b.y
    };
}

Vec2 Vec2Slerp(Vec2 a, Vec2 b, f32 t) {
    if(t < 0.01f) {
        return Vec2Lerp(a, b, t);
    }
    Vec2 from = Vec2Normalized(a);
    Vec2 to = Vec2Normalized(b);
    f32 theta = Vec2Angle(from, to);
    f32 sin_theta = sinf(theta);
    f32 s = sinf((1.0f - t) * theta) / sin_theta;
    f32 e = sinf(t * theta) / sin_theta;
    return from * s + to * s;
}

Vec2 Vec2Nlerp(Vec2 a, Vec2 b, f32 t) {
    Vec2 v = {
        (1 - t) * a.x + t * b.x,
        (1 - t) * a.y + t * b.y
    };
    return Vec2Normalized(v);
}

bool operator==(Vec2 a, Vec2 b) {
    Vec2 diff{a - b};
    return  Vec2LenSq(diff) < VEC_EPSILON;
}

bool operator!=(Vec2 a, Vec2 b) {
    return !(a == b);
}

struct Vec3 {
    union {
        struct {
            f32 x;
            f32 y;
            f32 z;
        };
        f32 v[3];
    };
    f32 operator[](u32 index) {
        return v[index];
    }
};

Vec3 operator+(Vec3 a, Vec3 b) 
{
    Vec3 result = {};
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

Vec3 operator-(Vec3 a, Vec3 b) {
    Vec3 result = {};
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

Vec3 operator-(Vec3 v) {
    Vec3 result = {};
    result.x = -v.x;
    result.y = -v.y;
    result.z = -v.z;
    return result;
}

Vec3 operator*(Vec3 a, Vec3 b) {
    Vec3 result{};
    result.x = a.x * b.x;
    result.y = a.y * b.y;
    result.z = a.z * b.z;
    return result;
}

Vec3 operator/(Vec3 a, Vec3 b) {
    Vec3 result = {};
    result.x = a.x / b.x;
    result.y = a.y / b.y;
    result.z = a.z / b.z;
    return result;
}

Vec3 operator*(Vec3 v, f32 s) {
    Vec3 result = {};
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return result;
}

Vec3 operator*(f32 s, Vec3 v) {
    Vec3 result = {};
    result.x = v.x * s;
    result.y = v.y * s;
    result.z = v.z * s;
    return result;
}

Vec3 operator/(Vec3 v, f32 s) {
    Vec3 result = {};
    result.x = v.x / s;
    result.y = v.y / s;
    result.z = v.z / s;
    return result;
}

f32 Vec3Dot(Vec3 a, Vec3 b) {
    f32 result = (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
    return result;
}

f32 Vec3LenSq(Vec3 v) {
    f32 result = Vec3Dot(v, v);
    return result;

}

f32 Vec3Len(Vec3 v) {
    f32 result = Vec3LenSq(v);
    if(result < VEC_EPSILON) {
        return 0.0f;
    }
    return sqrtf(result);
}

void Vec3Normalize(Vec3 *v) {
    f32 lenSq = Vec3LenSq(*v);
    if(lenSq < VEC_EPSILON) {
        return;
    }
    f32 invLen = 1.0f / sqrtf(lenSq);
    v->x *= invLen;
    v->y *= invLen;
    v->z *= invLen;
}

Vec3 Vec3Normalized(Vec3 v) {
    f32 lenSq = Vec3LenSq(v);
    if(lenSq < VEC_EPSILON) {
        return v;
    }
    f32 invLen = 1.0f / sqrtf(lenSq);
    Vec3 result{
        result.x = v.x * invLen,
        result.y = v.y * invLen,
        result.z = v.z * invLen,
    };
    return result;
}

f32 Vec3Angle(Vec3 a, Vec3 b) {
    f32 len = Vec3Len(a) * Vec3Len(b);
    if(len < VEC_EPSILON) {
        return 0.0f;
    }
    f32 dot = Vec3Dot(a, b);
    f32 result = acosf(dot / len);
    return result;
}

Vec3 Vec3Project(Vec3 a, Vec3 b) {
    f32 magB = Vec3Len(b);
    if(magB < VEC_EPSILON) {
        return {};
    }
    f32 scale = Vec3Dot(a, b) / magB;
    return b * scale;
}

Vec3 Vec3Reject(Vec3 a, Vec3 b) {
    Vec3 proj = Vec3Project(a, b);
    return a - proj;
}

Vec3 Vec3Reflect(Vec3 a, Vec3 b) {
    f32 magB = Vec3Len(b);
    if(magB < VEC_EPSILON) {
        return {};
    }
    f32 scale = Vec3Dot(a, b) / magB;
    Vec3 proj2 = b * (scale * 2.0f);
    return a - proj2;
}

Vec3 Vec3Cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,      
        a.z * b.x - a.x * b.z,      
        a.x * b.y - a.y * b.x
    };
}

Vec3 Vec3Lerp( Vec3 a, Vec3 b, f32 t) {
    return {
        (1 - t) * a.x + t * b.x,
        (1 - t) * a.y + t * b.y,
        (1 - t) * a.z + t * b.z
    };
}

Vec3 Vec3Slerp(Vec3 a, Vec3 b, f32 t) {
    if(t < 0.01f) {
        return Vec3Lerp(a, b, t);
    }
    Vec3 from = Vec3Normalized(a);
    Vec3 to = Vec3Normalized(b);
    f32 theta = Vec3Angle(from, to);
    f32 sin_theta = sinf(theta);
    f32 s = sinf((1.0f - t) * theta) / sin_theta;
    f32 e = sinf(t * theta) / sin_theta;
    return from * s + to * s;
}

Vec3 Vec3Nlerp(Vec3 a, Vec3 b, f32 t) {
    Vec3 v = {
        (1 - t) * a.x + t * b.x,
        (1 - t) * a.y + t * b.y,
        (1 - t) * a.z + t * b.z   
    };
    return Vec3Normalized(v);
}

bool operator==(Vec3 a, Vec3 b) {
    Vec3 diff{a - b};
    return  Vec3LenSq(diff) < VEC_EPSILON;
}

bool operator!=(Vec3 a, Vec3 b) {
    return !(a == b);
}



struct Vec4 {
    union {
        struct {
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };
        f32 v[4];
    };
    f32 operator[](u32 index)
    {
        return v[index];
    }
};

struct Mat3 
{
    union
    {
        f32 v[9];
		struct
        {
			f32 xx, xy, xz;
			f32 yx, yy, yz;
			f32 zx, zy, zz;
		};
    };
};

struct Mat4 
{
    union
    {
        f32 v[16];
        struct 
        {
            Vec4 right;
            Vec4 up;
            Vec4 forward;
            Vec4 position;
        };
		struct
        {
			f32 xx, xy, xz, xw;
			f32 yx, yy, yz, yw;
			f32 zx, zy, zz, zw;
			f32 tx, ty, tz, tw;
		};
    };
};

bool operator==(Mat4 a, Mat4 b) {
    for(int i = 0; i < 16; ++i) {
        if(fabsf(a.v[i] - b.v[i]) > MAT_EPSILON) {
            return false;
        }
    }
    return true;
}


bool operator!=(Mat4 a, Mat4 b) {
    return !(a == b);
}

Mat4 operator+(Mat4 a, Mat4 b) {
	return {
		a.xx + b.xx, a.xy + b.xy, a.xz + b.xz, a.xw + b.xw,
		a.yx + b.yx, a.yy + b.yy, a.yz + b.yz, a.yw + b.yw,
		a.zx + b.zx, a.zy + b.zy, a.zz + b.zz, a.zw + b.zw,
		a.tx + b.tx, a.ty + b.ty, a.tz + b.tz, a.tw + b.tw
    };
}

Mat4 operator*(Mat4 m, f32 f) {
    return {
        m.xx * f, m.xy * f, m.xz * f, m.xw * f,
        m.yx * f, m.yy * f, m.yz * f, m.yw * f,
        m.zx * f, m.zy * f, m.zz * f, m.zw * f,
        m.tx * f, m.ty * f, m.tz * f, m.tw * f
    };
}

Mat4 operator*(Mat4 a, Mat4 b) {
    Mat4 result {};
    
    for(int row = 0; row < 4; ++row) {
        for(int col = 0; col < 4; ++col) {
            result.v[row * 4 + col] =
                a.v[row * 4 + 0] * b.v[0 * 4 + col] +
                a.v[row * 4 + 1] * b.v[1 * 4 + col] +
                a.v[row * 4 + 2] * b.v[2 * 4 + col] +
                a.v[row * 4 + 3] * b.v[3 * 4 + col];
        }
    }

    return result;
}

Vec4 operator*(Mat4 m, Vec4 v) {
    Vec4 result{};
    
    for(int row = 0; row < 4; ++row) {
        result.v[row] = m.v[row * 4 + 0] * v.x + m.v[row * 4 + 1] * v.y + m.v[row * 4 + 2] * v.z + m.v[row * 4 + 3] * v.w;
    }

    return result;
}

Vec3 Mat4TransformVector(Mat4 m, Vec3 v) {
    Vec4 vec4 = {v.x, v.y, v.z, 0.0f};
    vec4 = m * vec4;
    Vec3 result = {vec4.x, vec4.y, vec4.z};
    return result;
}

Vec3 Mat4TransformPoint(Mat4 m, Vec3 v) {
    Vec4 vec4 = {v.x, v.y, v.z, 1.0f};
    vec4 = m * vec4;
    Vec3 result = {vec4.x, vec4.y, vec4.z};
    return result;
}

Vec3 Mat4TransformPoint(Mat4 m, Vec3 v, f32 *w) {
    f32 _w = *w;
    Vec4 vec4 = {v.x, v.y, v.z, _w};
    vec4 = m * vec4;
    *w = vec4.w;
    Vec3 result = {vec4.x, vec4.y, vec4.z};
    return result;
}

#define M4SWAP(x, y) \
    {f32 t = x; x = y; y = t; }


void Mat4Transpose(Mat4 *m) {
	M4SWAP(m->yx, m->xy);
	M4SWAP(m->zx, m->xz);
	M4SWAP(m->tx, m->xw);
	M4SWAP(m->zy, m->yz);
	M4SWAP(m->ty, m->yw);
	M4SWAP(m->tz, m->zw);
}

Mat4 Mat4Transposed(Mat4 m) {
	return {
		m.xx, m.yx, m.zx, m.tx,
		m.xy, m.yy, m.zy, m.ty,
		m.xz, m.yz, m.zz, m.tz,
		m.xw, m.yw, m.zw, m.tw
    };
}

#define M4_3X3MINOR(c0, c1, c2, r0, r1, r2) \
    (m.v[c0 * 4 + r0] * (m.v[c1 * 4 + r1] * m.v[c2 * 4 + r2] - m.v[c1 * 4 + r2] * m.v[c2 * 4 + r1]) - \
     m.v[c1 * 4 + r0] * (m.v[c0 * 4 + r1] * m.v[c2 * 4 + r2] - m.v[c0 * 4 + r2] * m.v[c2 * 4 + r1]) + \
     m.v[c2 * 4 + r0] * (m.v[c0 * 4 + r1] * m.v[c1 * 4 + r2] - m.v[c0 * 4 + r2] * m.v[c1 * 4 + r1]))

f32 Mat4Determinant(Mat4 m) {
	return  m.v[0] * M4_3X3MINOR(1, 2, 3, 1, 2, 3)
		  - m.v[4] * M4_3X3MINOR(0, 2, 3, 1, 2, 3)
		  + m.v[8] * M4_3X3MINOR(0, 1, 3, 1, 2, 3)
		  - m.v[12] * M4_3X3MINOR(0, 1, 2, 1, 2, 3);
}

Mat4 Mat4Adjugate(Mat4 m) {
	// Cofactor(M[i, j]) = Minor(M[i, j]] * pow(-1, i + j)
	Mat4 cofactor;

	cofactor.v[0] =  M4_3X3MINOR(1, 2, 3, 1, 2, 3);
	cofactor.v[1] = -M4_3X3MINOR(1, 2, 3, 0, 2, 3);
	cofactor.v[2] =  M4_3X3MINOR(1, 2, 3, 0, 1, 3);
	cofactor.v[3] = -M4_3X3MINOR(1, 2, 3, 0, 1, 2);

	cofactor.v[4] = -M4_3X3MINOR(0, 2, 3, 1, 2, 3);
	cofactor.v[5] =  M4_3X3MINOR(0, 2, 3, 0, 2, 3);
	cofactor.v[6] = -M4_3X3MINOR(0, 2, 3, 0, 1, 3);
	cofactor.v[7] =  M4_3X3MINOR(0, 2, 3, 0, 1, 2);

	cofactor.v[8] =   M4_3X3MINOR(0, 1, 3, 1, 2, 3);
	cofactor.v[9] =  -M4_3X3MINOR(0, 1, 3, 0, 2, 3);
	cofactor.v[10] =  M4_3X3MINOR(0, 1, 3, 0, 1, 3);
	cofactor.v[11] = -M4_3X3MINOR(0, 1, 3, 0, 1, 2);

	cofactor.v[12] = -M4_3X3MINOR(0, 1, 2, 1, 2, 3);
	cofactor.v[13] =  M4_3X3MINOR(0, 1, 2, 0, 2, 3);
	cofactor.v[14] = -M4_3X3MINOR(0, 1, 2, 0, 1, 3);
	cofactor.v[15] =  M4_3X3MINOR(0, 1, 2, 0, 1, 2);

	return Mat4Transposed(cofactor);
}

Mat4 Mat4Inverse(Mat4 m) {
	f32 det = Mat4Determinant(m);

	if (det == 0.0f) { // Epsilon check would need to be REALLY small
		printf("WARNING: Trying to invert a matrix with a zero determinant\n");
        ASSERT(!"INVALID_CODE_PATH");
	}
	Mat4 adj = Mat4Adjugate(m);

	return adj * (1.0f / det);
}

void Mat4Invert(Mat4 *m) {
	f32 det = Mat4Determinant(*m);
	if (det == 0.0f) {
		printf("WARNING: Trying to invert a matrix with a zero determinant\n");
        ASSERT(!"INVALID_CODE_PATH");
	}
	*m = Mat4Adjugate(*m) * (1.0f / det);
}

Mat4 Mat4Frustum(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    if (l == r || t == b || n == f) {
        printf("WARNING: Trying to create invalid frustum\n");
        ASSERT(!"INVALID_CODE_PATH");
    }

    return Mat4{
        (2.0f * n) / (r - l), 0, -(r + l) / (r - l), 0,
        0, (2.0f * n) / (t - b), -(t + b) / (t - b), 0,
        0, 0, f / (f - n), -(f * n) / (f - n),
        0, 0, 1, 0
    };
}

Mat4 Mat4Perspective(f32 fov, f32 aspect, f32 znear, f32 zfar) {
    f32 ymax = znear * tanf(fov * 3.14159265359f / 360.0f);
    f32 xmax = ymax * aspect;
    return Mat4Frustum(-xmax, xmax, -ymax, ymax, znear, zfar);
}

Mat4 Mat4Ortho(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    if (l == r || t == b || n == f) {
        ASSERT(!"INVALID_CODE_PATH");
    }
    
    Mat4 result = {
       2.0f / (r - l), 0, 0, -((r + l) / (r - l)),
       0, 2.0f / (t - b), 0, -(t + b) / (t - b),
       0, 0, 1.0f / (f - n), -(n / (f - n)),
       0, 0, 0, 1
    };

    return result;
}

Mat4 Mat4Identity() {

    return {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
    };
}

Mat4 Mat4LookAt(Vec3 position, Vec3 target, Vec3 up) {

    Vec3 zaxis = Vec3Normalized(target - position);
    Vec3 xaxis = Vec3Normalized(Vec3Cross(up, zaxis));
    Vec3 yaxis = Vec3Cross(zaxis, xaxis);
    Mat4 result = {
        xaxis.x, xaxis.y, xaxis.z, -Vec3Dot(xaxis, position), 
        yaxis.x, yaxis.y, yaxis.z, -Vec3Dot(yaxis, position), 
        zaxis.x, zaxis.y, zaxis.z, -Vec3Dot(zaxis, position),
        0,       0,       0,       1 
    };
    return result;
}

Mat4 Mat4Translate(f32 x, f32 y, f32 z) {
    Mat4 result = {
            1, 0, 0, x,
            0, 1, 0, y,
            0, 0, 1, z,
            0, 0, 0, 1
    };
    return result;
}

Mat4 Mat4Scale(f32 x, f32 y, f32 z) {
    Mat4 result = {
            x, 0, 0, 0,
            0, y, 0, 0,
            0, 0, z, 0,
            0, 0, 0, 1
    };
    return result;
}

Mat4 Mat4RotateX(f32 angle) {
    Mat4 result = {
            1,            0,           0,  0,
            0,  cosf(angle), sinf(angle),  0,
            0, -sinf(angle), cosf(angle),  0,
            0,            0,           0,  1
    };
    return result;
}

Mat4 Mat4RotateY(f32 angle) {
    Mat4 result = {
             cosf(angle), 0, -sinf(angle), 0,
                       0, 1,            0, 0,
             sinf(angle), 0,  cosf(angle), 0,
                       0, 0,            0, 1
    };
    return result;
}

Mat4 Mat4RotateZ(f32 angle) {
    Mat4 result = {
             cosf(angle), sinf(angle), 0, 0,
            -sinf(angle), cosf(angle), 0, 0,
                       0,           0, 1, 0,
                       0,           0, 0, 1
    };
    return result;
}

void PrintMatrix(Mat4 m)
{
    for(i32 j = 0; j < 4; ++j)
    {
        for(i32 i = 0; i < 4; ++i)
        {
            printf("%f ", m.v[j * 4 + i]);
        }
        printf("\n");
    }
}


struct Quat
{
    union
    {
        struct
        {
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };
        struct
        {
            Vec3 vector;
            f32 scala;
        };
        f32 v[4];
    };
};

#endif
