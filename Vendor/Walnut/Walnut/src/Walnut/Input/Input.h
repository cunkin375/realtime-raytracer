#pragma once

#include "KeyCodes.h"
#include "Walnut/Math/Vector.hpp"

#include <glm/glm.hpp>

namespace Walnut {

	class Input
	{
	public:
		static bool IsKeyDown(KeyCode keycode);
		static bool IsMouseButtonDown(MouseButton button);

		static glm::vec2 GetMousePosition();
		static fVector2 GetMousePositionCustom();

		static void SetCursorMode(CursorMode mode);
	};

}
