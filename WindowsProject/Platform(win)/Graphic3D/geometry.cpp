#include"GameCodeStd.h"
#include"geometry.h"

const DWORD D3D9Vertex_UnlitColored::FVF = (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_SPECULAR);
const DWORD D3D9Vertex_ColoredTextured::FVF = (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
const DWORD D3D9Vertex_Colored::FVF = (D3DFVF_XYZ | D3DFVF_DIFFUSE);
const DWORD D3D9Vertex_UnlitTextured::FVF= (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);

const Mat4x4 Mat4x4::s_Identity(*&D3DXMATRIX(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1));
const Quaternion Quaternion::s_Identity(*&D3DXQUATERNION(0, 0, 0, 1));

bool Plane::Inside(const Vec3& point) const
{
	float result = D3DXPlaneDotCoord(this, &point);
	return (result >= 0.0f);
}

bool Plane::Inside(const Vec3& point, const float radius) const
{
	float distance;
	distance = D3DXPlaneDotCoord(this, &point);
	return (distance >= -radius);
}

Frustum::Frustum()
{
	m_Fov = PI / 4.0f;
	m_Aspect = 16.0f / 9.0f;
	m_Near = 1.0f;
	m_Far = 1000.0f;
}

bool Frustum::Inside(const Vec3& point) const
{
	for (int i = 0; i < NumPlanes; ++i)
	{
		if (!m_Planes[i].Inside(point))
			return false;
	}
	return true;
}

bool Frustum::Inside(const Vec3& point, const float radius) const
{
	for (int i = 0; i < NumPlanes; ++i)
	{
		if (!m_Planes[i].Inside(point, radius))
		{
			return false;
		}
		return true;
	}
}

void Frustum::Init(const float fov, const float aspect, const float nearClip, const float farClip)
{
	m_Fov = fov;
	m_Aspect = aspect;
	m_Near = nearClip;
	m_Far = farClip;

	float tanFov = (float)tan(m_Fov / 2.0f);
	Vec3 nearRight = m_Near* tanFov* m_Aspect* g_Right;
	Vec3 farRight = m_Far * tanFov* m_Aspect* g_Right;
	Vec3 nearUp = m_Near * tanFov* g_Up;
	Vec3 farUp = m_Far * tanFov* g_Up;

	//Vertexes of the nearclip start in the upper left and go around clockwise
	m_NearClip[0] = (m_Near* g_Forward) - nearRight + nearUp;
	m_NearClip[1] = (m_Near* g_Forward) + nearRight + nearUp;
	m_NearClip[2] = (m_Near* g_Forward) + nearRight - nearUp;
	m_NearClip[3] = (m_Near* g_Forward) - nearRight - nearUp;

	//Vertexes of the farclip start in the upper left and go around clockwise
	m_FarClip[0] = (m_Far* g_Forward) - farRight + farUp;
	m_FarClip[1] = (m_Far* g_Forward) + farRight + farUp;
	m_FarClip[2] = (m_Far* g_Forward) + farRight - farUp;
	m_FarClip[3] = (m_Far* g_Forward) - farRight - farUp;

	//Construct 6 planes of the frustum with all 8 vertexes
	//and make sure the normals of every planes face toward inside
	m_Planes[Near].Init(m_NearClip[0], m_NearClip[2], m_NearClip[1]);
	m_Planes[Far].Init(m_FarClip[0], m_FarClip[1], m_FarClip[2]);
	m_Planes[Right].Init(m_FarClip[1], m_NearClip[1], m_NearClip[2]);
	m_Planes[Top].Init(m_NearClip[0], m_FarClip[1], m_FarClip[0]);
	m_Planes[Left].Init(m_NearClip[0], m_FarClip[0], m_FarClip[3]);
	m_Planes[Bottom].Init(m_FarClip[3], m_FarClip[2], m_NearClip[3]);
}

void Frustum::Render()
{
	D3D9Vertex_Colored verts[24];
	for (int i = 0; i < 8; ++i)
	{
		verts[i].color = g_White;
	}

	for (int i = 8; i < 16; ++i)
	{
		verts[i].color = g_Red;
	}

	for (int i = 16; i < 24; ++i)
	{
		verts[i].color = g_Blue;
	}

	//Draw the near clip plane
	verts[0].position = m_NearClip[0];	verts[1].position = m_NearClip[1];
	verts[2].position = m_NearClip[1];	verts[3].position = m_NearClip[2];
	verts[4].position = m_NearClip[2];	verts[5].position = m_NearClip[3];
	verts[6].position = m_NearClip[3];	verts[7].position = m_NearClip[0];

	//Draw the far clip plane
	verts[8].position = m_FarClip[0];	verts[9].position = m_FarClip[1];
	verts[10].position = m_FarClip[1];	verts[11].position = m_FarClip[2];
	verts[12].position = m_FarClip[2];	verts[13].position = m_FarClip[3];
	verts[14].position = m_FarClip[3];	verts[15].position = m_FarClip[0];

	//Draw the edges between the near and far clip plane
	verts[16].position = m_NearClip[0];	verts[17].position = m_FarClip[0];
	verts[18].position = m_NearClip[1];	verts[19].position = m_FarClip[1];
	verts[20].position = m_NearClip[2];	verts[21].position = m_FarClip[2];
	verts[22].position = m_NearClip[3];	verts[23].position = m_FarClip[3];

	DWORD oldLightMode;
	DXUTGetD3D9Device()->GetRenderState(D3DRS_LIGHTING, &oldLightMode);
	DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, FALSE);

	DXUTGetD3D9Device()->SetFVF(D3D9Vertex_Colored::FVF);
	DXUTGetD3D9Device()->DrawPrimitiveUP(D3DPT_LINELIST, 12, verts, sizeof(D3D9Vertex_Colored));
	
	DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, oldLightMode);
}

Vec3 BarycentricToVec3(Vec3 v0, Vec3 v1, Vec3 v2, float u, float v)
{
	Vec3 result;
	result = v0 + u * (v1 - v0) + v * (v2 - v0);
	return result;
}

//This function is based on "pick" demo of DirectX document
//If a known ray(given origin(orig) and direction(dir)) intersects a triangle, return true
//and outputs the texture coordinates of the intersection point
bool IntersectTriangle(const Vec3& orig, const Vec3& dir, Vec3& v0, 
						Vec3& v1, Vec3& v2, float* t,
						float* u, float* v)
{
	//Two edges sharing a same vertex
	Vec3 edge1 = v1 - v0;
	Vec3 edge2 = v2 - v0;

	//Determinat of three vectors(edge1 x edge2 * dir)
	Vec3 perpVec;
	D3DXVec3Cross(&perpVec, &dir, &edge2);
	float det = D3DXVec3Dot(&edge1, &perpVec);

	//If determinat is near 0, ray lies in plane of triangle
	Vec3 temp;
	if (det > 0)
	{
		temp = orig - v0;
	}
	else
	{
		temp = v0 - orig;
		det = -det;
	}
	if (det < 0.0001f)
		return false;

	//Calculate U parameter
	*u = D3DXVec3Dot(&temp, &perpVec);
	if (*u<0.0f || *u>det)
		return false;

	//Calculate V parameter
	D3DXVec3Cross(&perpVec, &temp, &edge1);
	*v = D3DXVec3Dot(&dir, &perpVec);
	if (*v<0.0f || *u + *v>det)
		return false;

	//Calculate t, scale parameters
	*t = D3DXVec3Dot(&edge2, &perpVec);
	float InvDet = 1.0f / det;
	*t *= InvDet;
	*u *= InvDet;
	*v *= InvDet;

	return true;

}

bool TriangleIterator::InitializeStrippedMesh(LPDIRECT3DVERTEXBUFFER9 pverts, int stride, int strips, int* triCountList)
{
	char* pvertices = NULL;
	if (FAILED(pverts->Lock(0, 0, (void**)&pvertices, 0)))
		return false;

	for (int i = 0; i < strips; ++i)
	{
		m_Size += triCountList[i];
	}

	m_Triangles = New Vec3[m_Size * 3];
	int src = 0;
	int dest = 0;

	for (int strip = 0; strip < strips; ++strip)
	{
		int vertsInStrip = triCountList[strip] + 2;
		ASSERT(vertsInStrip);

		m_Triangles[dest] = *((Vec3*)&pvertices[stride* src]);
		m_Triangles[dest+1] = *((Vec3*)&pvertices[stride* (src+1)]);
		m_Triangles[dest+2] = *((Vec3*)&pvertices[stride* (src+2)]);
		dest += 3;
		src += 3;
		for (int tri = 1; tri < triCountList[strip]; ++tri)
		{
			//Cast compressed triangle list to uncompressed triangle list
			//and keep the triangle at the same winding
			m_Triangles[dest] = m_Triangles[dest - 1];
			m_Triangles[dest + 1] = m_Triangles[dest - 2];
			m_Triangles[dest + 2] = *(Vec3*)&pvertices[stride* (src++)];
			dest += 3;

		}
	}
	ASSERT(dest == m_Size * 3);
	pverts->Unlock();

	return true;
}

void* TriangleIterator::VGet(unsigned int i)
{
	ASSERT(i < m_Size);
	return &m_Triangles[i * 3];
}



