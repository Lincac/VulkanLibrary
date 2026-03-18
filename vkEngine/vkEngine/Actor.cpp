#include "Actor.h"

#include <cstring>
#include <stdexcept>
#include <vector>

Actor::Actor()
	: _engine(nullptr),
	_vertexBuffer(VK_NULL_HANDLE),
	_vertexBufferMemory(VK_NULL_HANDLE),
	_indexBuffer(VK_NULL_HANDLE),
	_indexBufferMemory(VK_NULL_HANDLE),
	_materialsBuffer(VK_NULL_HANDLE),
	_materialsBufferMemory(VK_NULL_HANDLE),
	_materialsDescriptorPool(VK_NULL_HANDLE),
	_materialsDescriptorSetLayout(VK_NULL_HANDLE),
	_materialsDescriptorSet(VK_NULL_HANDLE)
{
	_updateMaterials = true;
	_materials.baseColorFactor = glm::vec4(1.0f);
	_materials.metallicFactor = 0.0f;
	_materials.roughnessFactor = 1.0f;
	_materials.normalScaleFactor = 1.0f;
	_materials.occlusionStrengthFactor = 1.0f;
	_materials.emissiveFactor = glm::vec3(0.0f);
	_materials.alphaCutoffFactor = 0.5f;
	_matrix = glm::mat4(1);
}

Actor::~Actor()
{
	releaseResources();
}

void Actor::init(vkEngine* engine)
{
	if (engine == nullptr)
	{
		return;
	}

	_engine = engine;

	initVertexData();
	initMaterialDescriptorSet();
}

void Actor::setInputData(const LoadedModel& data)
{
	size_t totalVertexCount = 0;
	size_t totalIndexCount = 0;
	for (const auto& mesh : data.meshes) {
		totalVertexCount += mesh.vertices.size();
		totalIndexCount += mesh.indices.size();
	}

	_vertices.reserve(totalVertexCount * 11); // pos(3) + normal(3) + tangent(3) + uv(2)
	_indices.reserve(totalIndexCount);

	uint32_t vertexBase = 0;
	for (const auto& mesh : data.meshes) {
		for (const auto& v : mesh.vertices) {
			_vertices.push_back(v.position.x);
			_vertices.push_back(v.position.y);
			_vertices.push_back(v.position.z);
			_vertices.push_back(v.normal.x);
			_vertices.push_back(v.normal.y);
			_vertices.push_back(v.normal.z);
			_vertices.push_back(v.tangent.x);
			_vertices.push_back(v.tangent.y);
			_vertices.push_back(v.tangent.z);
			_vertices.push_back(v.uv.x);
			_vertices.push_back(v.uv.y);
		}

		for (uint32_t localIndex : mesh.indices) {
			const uint32_t globalIndex = vertexBase + localIndex;
			_indices.push_back(globalIndex);
		}

		vertexBase += static_cast<uint32_t>(mesh.vertices.size());
	}
}

void Actor::setTransformMatrix(const glm::mat4& matrix)
{
	_matrix = matrix;
}

const glm::mat4 Actor::getTransformMatrix()
{
	return _matrix;
}

VkDescriptorSetLayout Actor::getMaterialDescriptorSetLayout() const
{
	return _materialsDescriptorSetLayout;
}

VkDescriptorSet Actor::getMaterialDescriptorSet()
{
	if (_engine == nullptr)
	{
		return VK_NULL_HANDLE;
	}

	if (!_updateMaterials)
	{
		return _materialsDescriptorSet;
	}

	void* data;
	vkMapMemory(_engine->getLogicalDevice(), _materialsBufferMemory, 0, sizeof(Materials), 0, &data);
	memcpy(data, &_materials, sizeof(Materials));
	vkUnmapMemory(_engine->getLogicalDevice(), _materialsBufferMemory);
	_updateMaterials = false;

	return _materialsDescriptorSet;
}

void Actor::draw(VkCommandBuffer command, VkPipelineLayout pipelineLayout, VkDescriptorSet cameraDescriptorSet)
{
	VkDescriptorSet materialDescriptorSet = getMaterialDescriptorSet();
	VkDescriptorSet descriptorSets[] = { cameraDescriptorSet, materialDescriptorSet };
	vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, descriptorSets, 0, nullptr);

	VkBuffer vertexBuffers[] = { _vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(command, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(command, _indexBuffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(command, _indices.size(), 1, 0, 0, 0);
}

void Actor::setBaseColor(const glm::vec4& color)
{
	_updateMaterials = true;
	_materials.baseColorFactor = color;
}

void Actor::setMetallic(float factor)
{
	_updateMaterials = true;
	_materials.metallicFactor = factor;
}

void Actor::setRoughness(float factor)
{
	_updateMaterials = true;
	_materials.roughnessFactor = factor;
}

void Actor::setNormalScale(float factor)
{
	_updateMaterials = true;
	_materials.normalScaleFactor = factor;
}

void Actor::setOcclusionStrength(float factor)
{
	_updateMaterials = true;
	_materials.occlusionStrengthFactor = factor;
}

void Actor::setEmissive(const glm::vec3& factor)
{
	_updateMaterials = true;
	_materials.emissiveFactor = factor;
}

void Actor::setAlphaCutoff(float factor)
{
	_updateMaterials = true;
	_materials.alphaCutoffFactor = factor;
}

void Actor::initVertexData()
{
	VkDevice device = _engine->getLogicalDevice();
	if (_vertexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, _vertexBuffer, nullptr);
		_vertexBuffer = VK_NULL_HANDLE;
	}
	if (_vertexBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, _vertexBufferMemory, nullptr);
		_vertexBufferMemory = VK_NULL_HANDLE;
	}
	if (_indexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, _indexBuffer, nullptr);
		_indexBuffer = VK_NULL_HANDLE;
	}
	if (_indexBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, _indexBufferMemory, nullptr);
		_indexBufferMemory = VK_NULL_HANDLE;
	}

	VkBuffer vertexStagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory vertexStagingMemory = VK_NULL_HANDLE;
	const VkDeviceSize vertexBufferSize = sizeof(float) * _vertices.size();
	_engine->createVKBuffer(
		vertexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertexStagingBuffer,
		vertexStagingMemory);

	void* mappedVertexData = nullptr;
	vkMapMemory(device, vertexStagingMemory, 0, vertexBufferSize, 0, &mappedVertexData);
	memcpy(mappedVertexData, _vertices.data(), static_cast<size_t>(vertexBufferSize));
	vkUnmapMemory(device, vertexStagingMemory);

	_engine->createVKBuffer(
		vertexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		_vertexBuffer,
		_vertexBufferMemory);

	_engine->copyBuffer(vertexStagingBuffer, _vertexBuffer, vertexBufferSize);
	vkDestroyBuffer(device, vertexStagingBuffer, nullptr);
	vkFreeMemory(device, vertexStagingMemory, nullptr);

	VkBuffer indexStagingBuffer = VK_NULL_HANDLE;
	VkDeviceMemory indexStagingMemory = VK_NULL_HANDLE;
	const VkDeviceSize indexBufferSize = sizeof(uint32_t) * _indices.size();
	_engine->createVKBuffer(
		indexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		indexStagingBuffer,
		indexStagingMemory);

	void* mappedIndexData = nullptr;
	vkMapMemory(device, indexStagingMemory, 0, indexBufferSize, 0, &mappedIndexData);
	memcpy(mappedIndexData, _indices.data(), static_cast<size_t>(indexBufferSize));
	vkUnmapMemory(device, indexStagingMemory);

	_engine->createVKBuffer(
		indexBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		_indexBuffer,
		_indexBufferMemory);

	_engine->copyBuffer(indexStagingBuffer, _indexBuffer, indexBufferSize);
	vkDestroyBuffer(device, indexStagingBuffer, nullptr);
	vkFreeMemory(device, indexStagingMemory, nullptr);
}

void Actor::initMaterialDescriptorSet()
{
	VkDeviceSize size = sizeof(Materials);

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vkCreateBuffer(_engine->getLogicalDevice(), &bufferInfo, nullptr, &_materialsBuffer);

	VkMemoryRequirements memReq;
	vkGetBufferMemoryRequirements(_engine->getLogicalDevice(), _materialsBuffer, &memReq);

	uint32_t memoryTypeIndex = _engine->findMemoryType(
		memReq.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex;

	vkAllocateMemory(_engine->getLogicalDevice(), &allocInfo, nullptr, &_materialsBufferMemory);
	vkBindBufferMemory(_engine->getLogicalDevice(), _materialsBuffer, _materialsBufferMemory, 0);

	VkDescriptorSetLayoutBinding uboBinding{};
	uboBinding.binding = 0;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.descriptorCount = 1;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboBinding;

	vkCreateDescriptorSetLayout(_engine->getLogicalDevice(), &layoutInfo, nullptr, &_materialsDescriptorSetLayout);

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = 1;

	vkCreateDescriptorPool(_engine->getLogicalDevice(), &poolInfo, nullptr, &_materialsDescriptorPool);

	VkDescriptorSetAllocateInfo descriptorAllocInfo{};
	descriptorAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorAllocInfo.descriptorPool = _materialsDescriptorPool;
	descriptorAllocInfo.descriptorSetCount = 1;
	descriptorAllocInfo.pSetLayouts = &_materialsDescriptorSetLayout;

	vkAllocateDescriptorSets(_engine->getLogicalDevice(), &descriptorAllocInfo, &_materialsDescriptorSet);

	VkDescriptorBufferInfo descriptorBufferInfo{};
	descriptorBufferInfo.buffer = _materialsBuffer;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = sizeof(Materials);

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = _materialsDescriptorSet;
	write.dstBinding = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &descriptorBufferInfo;

	vkUpdateDescriptorSets(_engine->getLogicalDevice(), 1, &write, 0, nullptr);
}

void Actor::releaseResources()
{
	if (_engine == nullptr) {
		return;
	}

	VkDevice device = _engine->getLogicalDevice();
	if (device == VK_NULL_HANDLE) {
		return;
	}

	if (_vertexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, _vertexBuffer, nullptr);
		_vertexBuffer = VK_NULL_HANDLE;
	}
	if (_vertexBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, _vertexBufferMemory, nullptr);
		_vertexBufferMemory = VK_NULL_HANDLE;
	}

	if (_indexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, _indexBuffer, nullptr);
		_indexBuffer = VK_NULL_HANDLE;
	}
	if (_indexBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, _indexBufferMemory, nullptr);
		_indexBufferMemory = VK_NULL_HANDLE;
	}

	if (_materialsBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, _materialsBuffer, nullptr);
		_materialsBuffer = VK_NULL_HANDLE;
	}
	if (_materialsBufferMemory != VK_NULL_HANDLE) {
		vkFreeMemory(device, _materialsBufferMemory, nullptr);
		_materialsBufferMemory = VK_NULL_HANDLE;
	}

	if (_materialsDescriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(device, _materialsDescriptorPool, nullptr);
		_materialsDescriptorPool = VK_NULL_HANDLE;
	}
	if (_materialsDescriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(device, _materialsDescriptorSetLayout, nullptr);
		_materialsDescriptorSetLayout = VK_NULL_HANDLE;
	}
}
