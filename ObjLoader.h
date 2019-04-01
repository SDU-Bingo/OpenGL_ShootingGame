#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>
#include <GL/glut.h>

class material
{
public:
	float Ka[4];		//ambient coefficient
	float Kd[4];		//diffuse coefficient
	float Ks[4];		//specular coefficient
	float Ns;			// shiness

	float Tr;			//Transpant (or d)
	std::string map_Ka;	//ambient texture
	std::string map_Kd;	//diffuse texture
	std::string map_Ks;	//specular texture

						//This mtl loader is still incomplete
						//Please see http://en.wikipedia.org/wiki/Wavefront_.obj_file#Material_template_library

	material()
	{
		for (size_t i = 0; i<4; i++)
			Ka[i] = Kd[i] = Ks[i] = 1.0f;
		Ns = 0.0f;
		Tr = 0.0f;
	}
};
class ObjLoader
{
	class Vertex		// 儲存基本 vertex 的 property
	{
	public:
		size_t v;		// vertex (index of vList)
		size_t n;		// normal (index of nList)
		size_t t;		// texture (index of tList)

		Vertex() {};
		Vertex(size_t v_index, size_t n_index, size_t t_index = 0)
		{
			v = v_index;
			n = n_index;
			t = t_index;
		}
	};
	class Vec3
	{
	public:
		float ptr[3];
		Vec3(float *v)
		{
			for (size_t i = 0; i < 3; i++)
				ptr[i] = v[i];
		}
		inline float& operator[](size_t index)
		{
			return ptr[index];
		}
	};

	class FACE			// faceList 的 structure
	{
	public:
		Vertex v[3];	// 3 vertex for each face
		int    m;		// material (index of material)

		FACE(Vertex &v1, Vertex &v2, Vertex &v3, int m_index)
		{
			v[0] = v1;
			v[1] = v2;
			v[2] = v3;
			m = m_index;
		}
		inline Vertex& operator[](size_t index)
		{
			return v[index];
		}
	};

	void	LoadMtl(std::string tex_file);
public:
	std::string					matFile;

	size_t						mTotal;	// total material 
	std::map<std::string, size_t>matMap;		// matMap[material_name] = material_ID
	std::vector<material>		mList;		// material ID (每個 mesh 最多有 100 種 material)	

	std::vector<Vec3>			vList;		// Vertex List (Position) - world cord.
	std::vector<Vec3>			nList;		// Normal List
	std::vector<Vec3>			tList;		// Texture List
	GLfloat* vArray;
	GLfloat* nArray;
	GLfloat* tArray;
	GLuint* fArray;
	std::vector<FACE>			faceList;	// Face List

	size_t	vTotal, tTotal, nTotal, fTotal; // number of total vertice, faces, texture coord., normal vecters, and faces
	
	void LoadObject(std::string obj_file);
	ObjLoader();
	ObjLoader(const char* obj_file);
	virtual ~ObjLoader();
	void Init(const char* obj_file);
private:
	FILE * fp_mtl;
	std::string s_file;
};