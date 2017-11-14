#ifndef HeapMeshH
#define HeapMeshH

#include "Ogre.h"

class HeapMesh
{
	public:
		HeapMesh(const Ogre::String& meshName, unsigned int meshSizeX, unsigned int meshSizeZ, Ogre::Real worldSizeX, Ogre::Real worldSizeZ, bool fakeNormals);
		virtual ~HeapMesh();

		void updateMesh(const float* heapMap);

	private:
		Ogre::String meshName;
		unsigned int meshSizeX;
		unsigned int meshSizeZ;
		size_t numFaces;
		size_t numVertices;
		Ogre::MeshPtr mesh;
		Ogre::SubMesh* subMesh;
		float* vertexBuffer;
		Ogre::Vector3* vNormals;
		bool fakeNormals;

		Ogre::HardwareVertexBufferSharedPtr posVertexBuffer;
		Ogre::HardwareVertexBufferSharedPtr normVertexBuffer;
		Ogre::HardwareVertexBufferSharedPtr texcoordsVertexBuffer;
		Ogre::HardwareIndexBufferSharedPtr indexBuffer;

		void calculateFakeNormals();
		void calculateNormals();
};

#endif
