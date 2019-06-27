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

// Miscellaneous
#include "Utils/Assert.h"
#include "Utils/MiscUtils.h"
#include <stdio.h>