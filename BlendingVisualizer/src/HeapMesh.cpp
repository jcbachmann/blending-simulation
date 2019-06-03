#include "detail/HeapMesh.h"

HeapMesh::HeapMesh(const Ogre::String& meshName, unsigned int meshSizeX, unsigned int meshSizeZ, Ogre::Real worldSizeX, Ogre::Real worldSizeZ, bool fakeNormals)
	: meshName(meshName)
	, meshSizeX(meshSizeX)
	, meshSizeZ(meshSizeZ)
	, vertexBuffer(nullptr)
	, fakeNormals(fakeNormals)
{
	numFaces = 2 * (size_t(meshSizeX) - 1) * (size_t(meshSizeZ) - 1);
	numVertices = size_t(meshSizeX) * size_t(meshSizeZ);

	// allocate space for normal calculation
	vNormals = new Ogre::Vector3[numVertices];

	// create mesh and submesh
	mesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	subMesh = mesh->createSubMesh();
	subMesh->useSharedVertices = false;

	// Vertex buffers
	subMesh->vertexData = new Ogre::VertexData();
	subMesh->vertexData->vertexStart = 0;
	subMesh->vertexData->vertexCount = numVertices;

	Ogre::VertexDeclaration* vdecl = subMesh->vertexData->vertexDeclaration;
	Ogre::VertexBufferBinding* vbind = subMesh->vertexData->vertexBufferBinding;

	vdecl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
	vdecl->addElement(1, 0, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
	vdecl->addElement(2, 0, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);

	// Prepare buffer for positions
	posVertexBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(3 * sizeof(float), numVertices,
		Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	vbind->setBinding(0, posVertexBuffer);

	// Prepare buffer for normals - write only
	normVertexBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(3 * sizeof(float), numVertices,
		Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	vbind->setBinding(1, normVertexBuffer);

	// Prepare texture coords buffer - static one
	{
		std::vector<float> texcoordsBufData(numVertices * 2);
		for (int z = 0; z < meshSizeZ; z++) {
			for (int x = 0; x < meshSizeX; x++) {
				texcoordsBufData[2 * (z * meshSizeX + x) + 0] = float(x) / float(meshSizeX - 1);
				texcoordsBufData[2 * (z * meshSizeX + x) + 1] = 1.0f - (float(z) / float(meshSizeZ - 1));
			}
		}
		texcoordsVertexBuffer =
			Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(2 * sizeof(float), numVertices, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
		texcoordsVertexBuffer->writeData(0, texcoordsVertexBuffer->getSizeInBytes(), texcoordsBufData.data(), true); // true?
		vbind->setBinding(2, texcoordsVertexBuffer);
	}

	// Prepare buffer for indices
	indexBuffer = Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(Ogre::HardwareIndexBuffer::IT_32BIT, 3 * numFaces,
		Ogre::HardwareBuffer::HBU_STATIC, true);
	auto* faceVertexIndices = (unsigned int*)indexBuffer->lock(0, numFaces * 3 * 4, Ogre::HardwareBuffer::HBL_DISCARD);
	for (int z = 0; z < meshSizeZ - 1; z++) {
		for (int x = 0; x < meshSizeX - 1; x++) {
			unsigned int* twoface = faceVertexIndices + (z * (meshSizeX - 1) + x) * 2 * 3;
			unsigned int p0 = z * meshSizeX + x;
			unsigned int p1 = z * meshSizeX + x + 1;
			unsigned int p2 = (z + 1) * meshSizeX + x;
			unsigned int p3 = (z + 1) * meshSizeX + x + 1;
			twoface[0] = p2; //first tri
			twoface[1] = p1;
			twoface[2] = p0;
			twoface[3] = p2; //second tri
			twoface[4] = p3;
			twoface[5] = p1;
		}
	}
	indexBuffer->unlock();
	// Set index buffer for this submesh
	subMesh->indexData->indexBuffer = indexBuffer;
	subMesh->indexData->indexStart = 0;
	subMesh->indexData->indexCount = 3 * numFaces;

	// prepare vertex positions
	vertexBuffer = new float[numVertices * 3];
	for (int z = 0; z < meshSizeZ; z++) {
		for (int x = 0; x < meshSizeX; x++) {
			int numPoint = z * meshSizeX + x;
			float* vertex = vertexBuffer + 3 * numPoint;
			vertex[0] = float(x) / float(meshSizeX - 1) * float(worldSizeX);
			vertex[1] = 0;
			vertex[2] = float(z) / float(meshSizeZ - 1) * float(worldSizeZ);
		}
	}

	Ogre::AxisAlignedBox meshBounds(0, 0, 0, worldSizeX, 0, worldSizeZ);
	mesh->_setBounds(meshBounds);

	posVertexBuffer->writeData(0, posVertexBuffer->getSizeInBytes(), vertexBuffer, true);

	mesh->load();
	mesh->touch();
}

HeapMesh::~HeapMesh()
{
	delete[] vertexBuffer;
	delete[] vNormals;

	Ogre::MeshManager::getSingleton().remove(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
}

void HeapMesh::calculateFakeNormals()
{
	float* buf = vertexBuffer + 1;
	auto* pNormals = (float*)normVertexBuffer->lock(0, normVertexBuffer->getSizeInBytes(), Ogre::HardwareBuffer::HBL_DISCARD);
	for (int z = 1; z < meshSizeZ - 1; z++) {
		float* nrow = pNormals + 3 * z * meshSizeX;
		float* row = buf + 3 * z * meshSizeX;
		float* rowup = buf + 3 * (z - 1) * meshSizeX;
		float* rowdown = buf + 3 * (z + 1) * meshSizeX;
		for (int x = 1; x < meshSizeX - 1; x++) {
			Ogre::Real xdiff = row[3 * x + 3] - row[3 * x - 3];
			Ogre::Real zdiff = rowup[3 * x] - rowdown[3 * x - 3];
			Ogre::Vector3 norm(xdiff, 30, zdiff);
			norm.normalise();
			nrow[3 * x + 0] = norm.x;
			nrow[3 * x + 1] = norm.y;
			nrow[3 * x + 2] = norm.z;
		}
	}
	normVertexBuffer->unlock();
}

void HeapMesh::calculateNormals()
{
	// zero normals
	for (int i = 0; i < numVertices; i++) {
		vNormals[i] = Ogre::Vector3::ZERO;
	}
	// first, calculate normals for faces, add them to proper vertices
	auto* vinds = (unsigned int*)indexBuffer->lock(0, indexBuffer->getSizeInBytes(), Ogre::HardwareBuffer::HBL_READ_ONLY);
	auto* pNormals = (float*)normVertexBuffer->lock(0, normVertexBuffer->getSizeInBytes(), Ogre::HardwareBuffer::HBL_DISCARD);
	for (int i = 0; i < numFaces; i++) {
		int p0 = vinds[3 * i];
		int p1 = vinds[3 * i + 1];
		int p2 = vinds[3 * i + 2];
		Ogre::Vector3 v0(vertexBuffer[3 * p0], vertexBuffer[3 * p0 + 1], vertexBuffer[3 * p0 + 2]);
		Ogre::Vector3 v1(vertexBuffer[3 * p1], vertexBuffer[3 * p1 + 1], vertexBuffer[3 * p1 + 2]);
		Ogre::Vector3 v2(vertexBuffer[3 * p2], vertexBuffer[3 * p2 + 1], vertexBuffer[3 * p2 + 2]);
		Ogre::Vector3 diff1 = v2 - v1;
		Ogre::Vector3 diff2 = v0 - v1;
		Ogre::Vector3 fn = diff1.crossProduct(diff2);
		vNormals[p0] += fn;
		vNormals[p1] += fn;
		vNormals[p2] += fn;
	}
	// now normalize vertex normals
	for (int z = 0; z < meshSizeZ; z++) {
		for (int x = 0; x < meshSizeX; x++) {
			int numPoint = z * meshSizeX + x;
			Ogre::Vector3 n = vNormals[numPoint];
			n.normalise();
			float* normal = pNormals + 3 * numPoint;
			normal[0] = n.x;
			normal[1] = n.y;
			normal[2] = n.z;
		}
	}
	indexBuffer->unlock();
	normVertexBuffer->unlock();
}

void HeapMesh::updateMesh(const float* heapMap)
{
	float* buf = vertexBuffer + 1; // +1 for Y coordinate

	for (int z = 0; z < meshSizeZ; z++) {
		float* row = buf + 3 * z * meshSizeX;
		for (int x = 0; x < meshSizeX; x++) {
			row[3 * x] = heapMap[z * meshSizeX + x] > 0.0 ? heapMap[z * meshSizeX + x] : -5.0f;
		}
	}

	if (fakeNormals) {
		calculateFakeNormals();
	} else {
		calculateNormals();
	}

	posVertexBuffer->writeData(0, posVertexBuffer->getSizeInBytes(), vertexBuffer, true);
}
