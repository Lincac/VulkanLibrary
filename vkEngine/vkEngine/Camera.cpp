#include "Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up)
	: Position(position), Up(up)
{
	this->Focal = glm::vec3(0);
	this->Resolution = glm::ivec2(1);
	this->Fov = 60.0f;
	this->NearPlane = 0.01f;
	this->FarPlane = 1000.0f;

	this->WindowCenter = glm::vec2(0);

	//UpdateCameraAttrs();
}

Camera::~Camera() = default;

void Camera::SetFocal(float _arg0, float _arg1, float _arg2)
{
	this->Focal = glm::vec3(_arg0, _arg1, _arg2);
}

void Camera::SetFocal(glm::vec3 focal)
{
	this->Focal = focal;
}

glm::vec3 Camera::GetFocal() const
{
	return this->Focal;
}

void Camera::SetPosition(float _arg0, float _arg1, float _arg2)
{
	this->Position = glm::vec3(_arg0, _arg1, _arg2);
}

void Camera::SetPosition(glm::vec3 position)
{
	this->Position = position;
}

glm::vec3 Camera::GetPosition() const
{
	return this->Position;
}

void Camera::SetResolution(int _arg0, int _arg1)
{
	this->Resolution = glm::ivec2(_arg0, _arg1);
}

void Camera::SetResolution(glm::ivec2 resolution)
{
	this->Resolution = resolution;
}

glm::ivec2 Camera::GetResolution() const
{
	return this->Resolution;
}

void Camera::SetFov(float fov)
{
	this->Fov = fov;
}

float Camera::GetFov() const
{
	return this->Fov;
}

void Camera::SetUp(float _arg0, float _arg1, float _arg2)
{
	this->Up = glm::vec3(_arg0, _arg1, _arg2);
}

void Camera::SetUp(glm::vec3 up)
{
	this->Up = up;
}

glm::vec3 Camera::GetUp() const
{
	return this->Up;
}

void Camera::SetWindowCenter(float _arg0, float _arg1)
{
	this->WindowCenter = glm::vec2(_arg0, _arg1);
}

void Camera::SetWindowCenter(glm::vec2 center)
{
	this->WindowCenter = center;
}

glm::vec2 Camera::GetWindowCenter() const
{
	return this->WindowCenter;
}

void Camera::ResetCamera()
{
	this->Position = glm::vec3(0, 0, 3);
	this->Up = glm::vec3(0, 1, 0);

	this->Focal = glm::vec3(0);
	this->Fov = 60.0f;
	this->NearPlane = 0.01f;
	this->FarPlane = 1000.0f;

	this->WindowCenter = glm::vec2(0);
}

void Camera::SetClipRange(float nearPlane, float farPlane)
{
	this->NearPlane = nearPlane;
	this->FarPlane = farPlane;
}

glm::vec2 Camera::GetClipRange()
{
	return glm::vec2(this->NearPlane, this->FarPlane);
}

float Camera::GetDistance()
{
	return glm::distance(this->Position, this->Focal);
}

glm::mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(this->Position, this->Focal, this->Up);
}

glm::mat4 Camera::GetProjectionMatrix(CameraType::Type type)
{
	switch (type)
	{
	case CameraType::PerspectiveAsym:
	{
		float halfHeight = this->NearPlane * tan(glm::radians(this->Fov) / 2.0f);
		float top = halfHeight + this->WindowCenter.x;
		float bottom = -halfHeight + this->WindowCenter.x;
		float halfWidth = halfHeight * (float)this->Resolution.x / (float)this->Resolution.y;
		float right = halfWidth + this->WindowCenter.y;
		float left = -halfWidth + this->WindowCenter.y;

		return glm::frustum(left, right, bottom, top, this->NearPlane, this->FarPlane);
	}
	case CameraType::Perspective:
		return glm::perspective(glm::radians(this->Fov), (float)this->Resolution.x / (float)this->Resolution.y,
			this->NearPlane, this->FarPlane);
	case CameraType::Orthogonal:
		return glm::ortho(-(float)this->Resolution.x / 2.0f, (float)this->Resolution.x / 2.0f,
			-(float)this->Resolution.y / 2.0f, (float)this->Resolution.y / 2.0f, this->NearPlane, this->FarPlane);
	default:
		return glm::mat4(1);
	}
}

glm::vec3 Camera::ScreenCoordToWorldCoord(glm::vec2 pos, CameraType::Type type)
{
	glm::mat4 view = GetViewMatrix();
	glm::mat4 projection = GetProjectionMatrix(type);

	glm::vec3 intersection = glm::vec3(0);

	if (type == CameraType::Perspective || type == CameraType::PerspectiveAsym)
	{
		glm::vec4 ndc_pos = glm::vec4(
			2.0f * pos.x / (float)this->Resolution.x - 1.0f,
			2.0f * pos.y / (float)this->Resolution.y - 1.0f,
			-1.0f,
			1.0f
		);

		glm::vec4 worldPos = glm::inverse(projection * view) * ndc_pos;
		worldPos = worldPos / worldPos.w;

		glm::vec3 ray = glm::normalize(glm::vec3(worldPos) - this->Position);

		glm::vec3 N = glm::normalize(this->Focal - this->Position);
		float d = -(this->Focal.x * N.x + this->Focal.y * N.y + this->Focal.z * N.z);

		float t = -(glm::dot(N, this->Position) + d) / glm::dot(N, ray);

		intersection = this->Position + t * ray;
	}
	else if (type == CameraType::Orthogonal)
	{
		glm::vec4 ndc_pos = glm::vec4(
			2.0f * pos.x / (float)this->Resolution.x - 1.0f,
			2.0f * pos.y / (float)this->Resolution.y - 1.0f,
			0.0f,
			1.0f
		);

		glm::vec4 worldPos = glm::inverse(projection * view) * ndc_pos;

		intersection = glm::vec3(worldPos);
	}

	return intersection;
}

glm::vec2 Camera::WorldCoordToScreenCoord(glm::vec3 pos, CameraType::Type type)
{
	glm::mat4 view = GetViewMatrix();
	glm::mat4 projection = GetProjectionMatrix(type);

	glm::vec4 clipSpacePos = projection * view * glm::vec4(pos, 1.0f);

	glm::vec3 ndcSpacePos;
	if (type == CameraType::Perspective || type == CameraType::PerspectiveAsym)
	{
		ndcSpacePos = glm::vec3(clipSpacePos) / clipSpacePos.w;
	}
	else
	{
		ndcSpacePos = glm::vec3(clipSpacePos);
	}

	glm::vec2 screenSpacePos;
	screenSpacePos.x = (ndcSpacePos.x * 0.5f + 0.5f) * Resolution.x;
	screenSpacePos.y = (1.0f - (ndcSpacePos.y * 0.5f + 0.5f)) * Resolution.y;

	return screenSpacePos;
}

#include <spdlog/spdlog.h>
void Camera::Forward(float delta)
{
	glm::vec3 direction = glm::normalize(this->Focal - this->Position);

	// 计算当前距离
	float currentDistance = glm::distance(this->Focal, this->Position);
	
	// 计算最小允许距离
	// 基于 NearPlane 和 Fov 计算一个合理的最小距离
	// 使用 NearPlane 的倍数作为安全距离，确保相机不会太接近焦点
	// 同时考虑 Fov，视野越大，最小距离可以稍微放宽
	float minDistanceFactor = 1.5f;  // 安全系数，确保距离至少是 NearPlane 的 1.5 倍
	float fovFactor = 1.0f + (this->Fov - 60.0f) / 120.0f * 0.5f;  // 根据 Fov 调整（60度时为1.0，范围0.75-1.25）
	float minDistance = this->NearPlane * minDistanceFactor * fovFactor;
	
	// 如果当前距离已经小于或等于最小距离，不允许继续前进
	if (currentDistance <= minDistance)
	{
		return;
	}
	
	// 计算新位置
	glm::vec3 newPosition = this->Position + direction * delta;
	float newDistance = glm::distance(this->Focal, newPosition);
	
	// 如果新距离小于最小距离，限制到最小距离
	if (newDistance < minDistance)
	{
		newPosition = this->Focal - direction * minDistance;
	}
	
	// 确保新位置不会越过焦点（额外的安全检查）
	float val = glm::dot(this->Focal - newPosition, direction);
	if (val <= 0.0f)
	{
		return;
	}

	this->Position = newPosition;
}

void Camera::Backward(float delta)
{
	glm::vec3 direction = glm::normalize(this->Focal - this->Position);
	this->Position -= direction * delta;
}

void Camera::Rotate2D(float angle)
{
	glm::vec3 front = glm::normalize(this->Focal - this->Position);
	glm::quat rotateQuat = glm::angleAxis(glm::radians(angle), front);
	this->Up = glm::normalize(rotateQuat * this->Up);
}

void Camera::Rotate3D(glm::vec2 delta)
{
	float distance = glm::distance(this->Position, this->Focal);

	glm::vec3 front = glm::normalize(this->Focal - this->Position);
	glm::vec3 right = glm::normalize(glm::cross(front, this->Up));
	glm::vec3 currentUp = glm::normalize(glm::cross(right, front));

	glm::quat currentQuat = glm::quatLookAt(front, this->Up);

	glm::quat pitchQuat = glm::angleAxis(glm::radians(delta.y), right);
	glm::quat yawQuat = glm::angleAxis(glm::radians(-delta.x), currentUp);

	glm::quat deltaQuat = yawQuat * pitchQuat;
	glm::quat newQuat = deltaQuat * currentQuat;

	glm::vec3 newFront = glm::rotate(newQuat, glm::vec3(0.0f, 0.0f, -1.0f));
	glm::vec3 newUp = glm::rotate(newQuat, glm::vec3(0.0f, 1.0f, 0.0f));

	// 在旋转前，将 WindowCenter 从旧的视图空间转换到新的视图空间
	// WindowCenter 是相对于视图空间的，当视图空间改变时，需要转换
	// 使用视图矩阵来确保正确的坐标转换
	glm::vec2 oldWindowCenter = this->WindowCenter;
	
	// 获取旧的视图矩阵（在旋转前）
	glm::mat4 oldViewMatrix = this->GetViewMatrix();
	
	// 计算旧视图空间的基向量（从视图矩阵中提取）
	// 视图矩阵的列向量是视图空间的基向量（在世界空间中）
	// glm 矩阵访问：matrix[col][row]
	// 从 lookAt 实现看：Result[0][0]=s.x, Result[1][0]=s.y, Result[2][0]=s.z (第一列是右向量)
	// Result[0][1]=u.x, Result[1][1]=u.y, Result[2][1]=u.z (第二列是上向量)
	glm::vec3 oldRight = glm::normalize(glm::vec3(oldViewMatrix[0][0], oldViewMatrix[1][0], oldViewMatrix[2][0]));
	glm::vec3 oldViewUp = glm::normalize(glm::vec3(oldViewMatrix[0][1], oldViewMatrix[1][1], oldViewMatrix[2][1]));
	
	// 将 WindowCenter 从旧视图空间转换到世界空间（在近平面上）
	// WindowCenter.y 对应 X 轴（右向量，水平方向）
	// WindowCenter.x 对应 Y 轴（上向量，垂直方向）
	glm::vec3 worldOffset = oldRight * oldWindowCenter.y + oldViewUp * oldWindowCenter.x;
	
	// 临时更新相机状态以计算新视图矩阵
	glm::vec3 oldPosition = this->Position;
	glm::vec3 oldUp = this->Up;
	this->Position = this->Focal - newFront * distance;
	this->Up = newUp;
	
	// 获取新的视图矩阵
	glm::mat4 newViewMatrix = this->GetViewMatrix();
	
	// 计算新视图空间的基向量
	// glm 矩阵访问：matrix[col][row]
	// 从 lookAt 实现看：Result[0][0]=s.x, Result[1][0]=s.y, Result[2][0]=s.z (第一列是右向量)
	// Result[0][1]=u.x, Result[1][1]=u.y, Result[2][1]=u.z (第二列是上向量)
	glm::vec3 newRight = glm::normalize(glm::vec3(newViewMatrix[0][0], newViewMatrix[1][0], newViewMatrix[2][0]));
	glm::vec3 newViewUp = glm::normalize(glm::vec3(newViewMatrix[0][1], newViewMatrix[1][1], newViewMatrix[2][1]));
	
	// 将世界空间偏移投影到新视图空间的基向量上
	glm::vec2 newWindowCenter;
	newWindowCenter.y = glm::dot(worldOffset, newRight);  // X 轴（水平方向）
	newWindowCenter.x = glm::dot(worldOffset, newViewUp); // Y 轴（垂直方向）
	
	this->WindowCenter = newWindowCenter;
}

void Camera::UpdateCameraAttrs()
{
	glm::vec3 front = glm::normalize(this->Focal - this->Position);
	glm::vec3 worldUP = glm::vec3(0, 1, 0);
	if (glm::abs(glm::dot(worldUP, -front)) > .9f) worldUP = glm::vec3(0, 0, 1);
	glm::vec3 right = glm::normalize(glm::cross(front, worldUP));

	this->Up = glm::normalize(glm::cross(right, front));
}