#pragma once

// Windows
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// ATL
#include <atlbase.h>
#include <atlwin.h>
#include <atlstr.h>
#include <atltypes.h>

// STL
#include <array>
#include <vector>
#include <algorithm>
#include <unordered_map>

// OpenGL
#include "Utils/glew/glew.h"
#include "Utils/glew/wglew.h"

// Direct3D
#include <d3d11.h>
#include <DirectXMath.h>
#include "Utils/dxtk/SimpleMath.h"

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Matrix;

// Miscellaneous
#include "Utils/Assert.h"
#include "Utils/MiscUtils.h"
#include <stdio.h>