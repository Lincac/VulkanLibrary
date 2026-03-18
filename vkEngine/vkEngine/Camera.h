#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

/**
 * @brief 相机类型命名空间
 */
namespace CameraType
{
	/**
	 * @brief 相机类型枚举
	 */
	enum Type
	{
		Perspective,      ///< 透视投影
		PerspectiveAsym,  ///< 非对称透视投影
		Orthogonal        ///< 正交投影
	};
};

/**
 * @brief 相机类
 * 
 * 管理场景相机，提供视图矩阵和投影矩阵的计算。
 * 支持透视投影、非对称透视投影和正交投影。
 * 提供坐标转换功能（屏幕坐标 ↔ 世界坐标）。
 */
class Camera
{
public:

	/**
	 * @brief 构造函数
	 * @param position 相机位置，默认 (0, 0, 3)
	 * @param up 相机上方向，默认 (0, 1, 0)
	 */
	Camera(glm::vec3 position = glm::vec3(0, 0, 3), glm::vec3 up = glm::vec3(0, 1, 0));
	
	/**
	 * @brief 析构函数
	 */
	~Camera();

	/**
	 * @brief 设置焦点（三个参数版本）
	 * @param _arg0 X 坐标
	 * @param _arg1 Y 坐标
	 * @param _arg2 Z 坐标
	 */
	void SetFocal(float _arg0, float _arg1, float _arg2);
	
	/**
	 * @brief 设置焦点
	 * @param focal 三维焦点向量
	 */
	void SetFocal(glm::vec3 focal);
	
	/**
	 * @brief 获取焦点
	 * @return 返回焦点向量
	 */
	glm::vec3 GetFocal() const;

	/**
	 * @brief 设置相机位置（三个参数版本）
	 * @param _arg0 X 坐标
	 * @param _arg1 Y 坐标
	 * @param _arg2 Z 坐标
	 * 
	 * 在世界坐标系中设置相机的位置
	 */
	void SetPosition(float _arg0, float _arg1, float _arg2);
	
	/**
	 * @brief 设置相机位置
	 * @param position 三维位置向量
	 */
	void SetPosition(glm::vec3 position);
	
	/**
	 * @brief 获取相机位置
	 * @return 返回位置向量
	 */
	glm::vec3 GetPosition() const;

	/**
	 * @brief 设置屏幕空间分辨率（两个参数版本）
	 * @param _arg0 宽度
	 * @param _arg1 高度
	 */
	void SetResolution(int _arg0, int _arg1);
	
	/**
	 * @brief 设置屏幕空间分辨率
	 * @param resolution 二维分辨率向量 [width, height]
	 */
	void SetResolution(glm::ivec2 resolution);
	
	/**
	 * @brief 获取屏幕空间分辨率
	 * @return 返回分辨率向量
	 */
	glm::ivec2 GetResolution() const;

	/**
	 * @brief 设置视野角度
	 * @param fov 视野角度（度），默认 60.0f
	 */
	void SetFov(float fov);
	
	/**
	 * @brief 获取视野角度
	 * @return 返回视野角度值
	 */
	float GetFov() const;

	/**
	 * @brief 设置相机上方向（三个参数版本）
	 * @param _arg0 X 分量
	 * @param _arg1 Y 分量
	 * @param _arg2 Z 分量
	 */
	void SetUp(float _arg0, float _arg1, float _arg2);
	
	/**
	 * @brief 设置相机上方向
	 * @param up 三维上方向向量
	 */
	void SetUp(glm::vec3 up);
	
	/**
	 * @brief 获取相机当前上方向
	 * @return 返回上方向向量
	 */
	glm::vec3 GetUp() const;

	/**
	 * @brief 设置窗口中心（两个参数版本）
	 * @param _arg0 X 坐标
	 * @param _arg1 Y 坐标
	 */
	void SetWindowCenter(float _arg0, float _arg1);
	
	/**
	 * @brief 设置窗口中心
	 * @param center 二维中心向量
	 */
	void SetWindowCenter(glm::vec2 center);
	
	/**
	 * @brief 获取窗口中心
	 * @return 返回中心向量
	 */
	glm::vec2 GetWindowCenter() const;

	/**
	 * @brief 重置相机
	 * 
	 * 将相机恢复到初始状态
	 */
	void ResetCamera();

	/**
	 * @brief 设置裁剪范围
	 * @param nearPlane 近平面距离
	 * @param farPlane 远平面距离
	 * 
	 * 相机近远平面的默认值为 [0.1, 1000.0]
	 */
	void SetClipRange(float nearPlane, float farPlane);
	
	/**
	 * @brief 获取裁剪范围
	 * @return 返回二维向量 [nearPlane, farPlane]
	 */
	glm::vec2 GetClipRange();

	float GetDistance();

	/**
	 * @brief 获取视图矩阵
	 * @return 返回 4x4 视图矩阵
	 */
	glm::mat4 GetViewMatrix();

	/**
	 * @brief 获取投影矩阵
	 * @param type 投影类型（透视、非对称透视、正交）
	 * @return 返回 4x4 投影矩阵
	 */
	glm::mat4 GetProjectionMatrix(CameraType::Type type);

	/**
	 * @brief 屏幕坐标转世界坐标
	 * @param pos 屏幕空间中的鼠标坐标位置
	 * @param type 投影类型
	 * @return 返回世界空间坐标
	 * 
	 * 输入屏幕空间中的坐标位置，返回世界空间中的坐标。
	 * 平面是相机焦点所在的平面。
	 * 根据所需的投影方法获取世界空间坐标。
	 */
	glm::vec3 ScreenCoordToWorldCoord(glm::vec2 pos, CameraType::Type type);

	/**
	 * @brief 世界坐标转屏幕坐标
	 * @param pos 世界空间坐标
	 * @param type 投影类型
	 * @return 返回屏幕空间坐标
	 */
	glm::vec2 WorldCoordToScreenCoord(glm::vec3 pos, CameraType::Type type);

	/**
	 * @brief 向前移动
	 * @param delta 移动距离
	 */
	void Forward(float delta);

	/**
	 * @brief 向后移动
	 * @param delta 移动距离
	 */
	void Backward(float delta);

	/**
	 * @brief 二维旋转
	 * @param angle 旋转角度，默认 90.0f
	 * 
	 * 在当前平面内围绕 Front 向量旋转相机
	 */
	void Rotate2D(float angle = 90.0f);

	/**
	 * @brief 三维旋转
	 * @param delta 屏幕 xy 坐标的差值
	 * 
	 * 输入屏幕 xy 坐标的差值，围绕相机的焦点旋转 360°
	 */
	void Rotate3D(glm::vec2 delta);

private:

	/**
	 * @brief 更新相机属性
	 * 
	 * 内部方法，用于更新相机相关属性
	 */
	void UpdateCameraAttrs();

private:

	glm::vec3 Focal;      ///< 焦点位置
	glm::vec3 Position;  ///< 相机位置
	glm::vec3 Up;         ///< 上方向
	glm::ivec2 Resolution;  ///< 屏幕分辨率

	glm::vec2 WindowCenter;  ///< 窗口中心

	float NearPlane;  ///< 近平面距离
	float FarPlane;   ///< 远平面距离
	float Fov;        ///< 视野角度

private:
	Camera(const Camera&) = delete;
	void operator=(const Camera&) = delete;
};
