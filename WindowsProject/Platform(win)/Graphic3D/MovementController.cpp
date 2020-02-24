#include"GameCodeStd.h"
#include"geometry.h"
#include"MovementController.h"
#include"SceneNode.h"



#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


MovementController::MovementController(shared_ptr<SceneNode> object, float initialYaw, float initialPitch, bool rotateWhenLButtonDown)
	: m_object(object)
{
	m_object->VGet()->Transform(&m_matToWorld, &m_matFromWorld);

	m_fTargetYaw = m_fYaw = RADIANS_TO_DEGREES(-initialYaw);
	m_fTargetPitch = m_fPitch = RADIANS_TO_DEGREES(initialPitch);

	m_maxSpeed = 30.0f;			// 30 meters per second
	m_currentSpeed = 0.0f;

	Vec3 pos = m_matToWorld.GetPosition();

	m_matPosition.BuildTranslation(pos);

	POINT ptCursor;
	GetCursorPos(&ptCursor);
	m_lastMousePos = ptCursor;

	memset(m_bKey, 0x00, sizeof(m_bKey));

	m_mouseLButtonDown = false;
	m_bRotateWhenLButtonDown = rotateWhenLButtonDown;
}



bool MovementController::VOnPointerButtonDown(const Point &mousePos, const int radius, const std::string &buttonName)
{
	if (buttonName == "PointerLeft")
	{
		m_mouseLButtonDown = true;

		// Register mouse position when pressed
		m_lastMousePos = mousePos;
		return true;
	}
	return false;
}

bool MovementController::VOnPointerButtonUp(const Point &mousePos, const int radius, const std::string &buttonName)
{
	if (buttonName == "PointerLeft")
	{
		m_mouseLButtonDown = false;
		return true;
	}
	return false;
}





bool MovementController::VOnPointerMove(const Point &mousePos, const int radius)
{
	
	if (m_bRotateWhenLButtonDown)
	{
		
		if (m_lastMousePos != mousePos && m_mouseLButtonDown)
		{
			m_fTargetYaw = m_fTargetYaw + (m_lastMousePos.x - mousePos.x);
			m_fTargetPitch = m_fTargetPitch + (mousePos.y - m_lastMousePos.y);
			m_lastMousePos = mousePos;
		}
	}
	else if (m_lastMousePos != mousePos)
	{
		
		m_fTargetYaw = m_fTargetYaw + (m_lastMousePos.x - mousePos.x);
		m_fTargetPitch = m_fTargetPitch + (mousePos.y - m_lastMousePos.y);
		m_lastMousePos = mousePos;
	}

	return true;
}


void MovementController::OnUpdate(DWORD const deltaMilliseconds)
{

	bool bTranslating = false;
	Vec4 atWorld(0, 0, 0, 0);
	Vec4 rightWorld(0, 0, 0, 0);
	Vec4 upWorld(0, 0, 0, 0);

	if (m_bKey['W'] || m_bKey['S'])
	{
		// Default lookAt vector
		Vec4 at = g_Forward4;
		if (m_bKey['S'])
			at *= -1;

		// Get lookAt vector in world space
		atWorld = m_matToWorld.Xform(at);
		bTranslating = true;
	}

	if (m_bKey['A'] || m_bKey['D'])
	{
		//Default right vector
		Vec4 right = g_Right4;
		if (m_bKey['A'])
			right *= -1;

		// Get right vector in world space
		rightWorld = m_matToWorld.Xform(right);
		bTranslating = true;
	}

	if (m_bKey[' '] || m_bKey['C'] || m_bKey['X'])
	{
		// Default up vector
		Vec4 up = g_Right4;
		if (!m_bKey[' '])
			up *= -1;

		//Up is always up
		upWorld = up;
		bTranslating = true;
	}

	//Handling rotation as a result of mouse position
	{
		
		m_fYaw += (m_fTargetYaw - m_fYaw) * (.35f);
		m_fTargetPitch = MAX(-90, MIN(90, m_fTargetPitch));
		m_fPitch += (m_fTargetPitch - m_fPitch) * (.35f);

		// Calculate the new rotation matrix from the camera
		// yaw and pitch.
		Mat4x4 matRot;
		matRot.BuildYawPitchRoll(DEGREES_TO_RADIANS(-m_fYaw), DEGREES_TO_RADIANS(m_fPitch), 0);

		// Create the new object-to-world matrix, and the
		// new world-to-object matrix. 

		m_matToWorld = matRot * m_matPosition;
		m_matFromWorld = m_matToWorld.Inverse();
		m_object->VSetTransform(&m_matToWorld, &m_matFromWorld);
	}

	if (bTranslating)
	{
		float elapsedTime = (float)deltaMilliseconds / 1000.0f;

		Vec3 direction = atWorld + rightWorld + upWorld;
		direction.Normalize();

		// Ramp the acceleration by the elapsed time.
		float numberOfSeconds = 5.f;
		m_currentSpeed += m_maxSpeed * ((elapsedTime*elapsedTime) / numberOfSeconds);
		if (m_currentSpeed > m_maxSpeed)
			m_currentSpeed = m_maxSpeed;

		direction *= m_currentSpeed;

		Vec3 pos = m_matPosition.GetPosition() + direction;
		m_matPosition.SetPosition(pos);
		m_matToWorld.SetPosition(pos);

		m_matFromWorld = m_matToWorld.Inverse();
		m_object->VSetTransform(&m_matToWorld, &m_matFromWorld);
	}
	else
	{
		m_currentSpeed = 0.0f;
	}
}