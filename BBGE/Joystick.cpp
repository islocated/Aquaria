/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "Core.h"

#if defined(BBGE_BUILD_WINDOWS) && defined(BBGE_BUILD_XINPUT) 
	#include "Xinput.h"

#if defined(BBGE_BUILD_DELAYXINPUT)
	#include <DelayImp.h>
#endif

/*
	HRESULT (WINAPI *XInputGetState)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter) = 0;

if ( (winp.hInstDI = LoadLibrary( "dinput.dll" )) == 0 )


if (!pDirectInput8Create) {
	pDirectInput8Create = (HRESULT (__stdcall *)(HINSTANCE, DWORD ,REFIID, LPVOID *, LPUNKNOWN)) GetProcAddress(winp.hInstDI,"DirectInput8Create");

	if (!pDirectInput8Create) {
		error(L"Couldn't get DI proc addr\n");
	}
} 

	bool importXInput()
	{

	}
*/




bool tryXInput()
{
	__try
	{
		XINPUT_STATE xinp;
		XInputGetState(0, &xinp);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
	return true;
}

#endif

Joystick::Joystick()
{
	xinited = false;
	stickIndex = -1;
#ifdef BBGE_BUILD_SDL
	sdl_joy = 0;
#endif
	inited = false;
	for (int i = 0; i < maxJoyBtns; i++)
	{
		buttons[i] = UP;
	}
	deadZone1 = 0.3;
	deadZone2 = 0.3;

	clearRumbleTime= 0;
	leftThumb = rightThumb = false;
	leftTrigger = rightTrigger = 0;
	rightShoulder = leftShoulder = false;
	dpadRight = dpadLeft = dpadUp = dpadDown = false;
	btnStart = false;
	btnSelect = false;

	s1ax = 0;
	s1ay = 1;
	s2ax = 4;
	s2ay = 3;
}

void Joystick::init(int stick)
{
#ifdef BBGE_BUILD_SDL
	stickIndex = stick;
	int numJoy = SDL_NumJoysticks();
	std::ostringstream os;
	os << "Found [" << numJoy << "] joysticks";
	debugLog(os.str());
	if (numJoy > stick)
	{
		sdl_joy = SDL_JoystickOpen(stick);
		if (sdl_joy)
		{
			inited = true;
			debugLog(std::string("Initialized Joystick [") + std::string(SDL_JoystickName(stick)) + std::string("]"));
		}
		else
		{
			std::ostringstream os;
			os << "Failed to init Joystick [" << stick << "]";
			debugLog(os.str());
		}
	}
	else
	{
		debugLog("Not enough Joystick(s) found");
	}
#endif

#ifdef BBGE_BUILD_XINPUT
	debugLog("about to init xinput");

	xinited = tryXInput();

	if (!xinited)
		debugLog("XInput not found, not installed?");

	debugLog("after catch");

#if !defined(BBGE_BUILD_SDL)
	inited = xinited;
#endif
#endif
}

void Joystick::rumble(float leftMotor, float rightMotor, float time)
{
	if (core->joystickEnabled && inited)
	{
#if defined(BBGE_BUILD_WINDOWS) && defined(BBGE_BUILD_XINPUT)
		XINPUT_VIBRATION vib;
		vib.wLeftMotorSpeed = WORD(leftMotor*65535);
		vib.wRightMotorSpeed = WORD(rightMotor*65535);
		
		clearRumbleTime = time;
		DWORD d = XInputSetState(0, &vib);
		if (d == ERROR_SUCCESS)
		{
			//debugLog("success");
		}
		else if (d == ERROR_DEVICE_NOT_CONNECTED)
		{
			//debugLog("joystick not connected");
		}
		else
		{
			//unknown error
		}
#endif
	}
}

void Joystick::callibrate(Vector &calvec, float deadZone)
{
	//float len = position.getLength2D();
	if (calvec.isLength2DIn(deadZone))
	{
		calvec = Vector(0,0,0);
	}
	else
	{
		if (!calvec.isZero())
		{				
			Vector pos2 = calvec;
			pos2.setLength2D(deadZone);
			calvec -= pos2;
			float mult = 1.0/float(1.0-deadZone);
			calvec.x *= mult;
			calvec.y *= mult;
			if (calvec.x > 1)
				calvec.x = 1;
			else if (calvec.x < -1)
				calvec.x = -1;

			if (calvec.y > 1)
				calvec.y = 1;
			else if (calvec.y < -1)
				calvec.y = -1;
		}
	}
}

void Joystick::update(float dt)
{	
#ifdef BBGE_BUILD_SDL
	if (core->joystickEnabled && inited && sdl_joy && stickIndex != -1)
	{	
		if (!SDL_JoystickOpened(stickIndex))
		{
			debugLog("Lost Joystick");
			sdl_joy = 0;
			return;
		}

		Sint16 xaxis = SDL_JoystickGetAxis(sdl_joy, s1ax);
		Sint16 yaxis = SDL_JoystickGetAxis(sdl_joy, s1ay);
		position.x = double(xaxis)/32768.0;
		position.y = double(yaxis)/32768.0;


		Sint16 xaxis2 = SDL_JoystickGetAxis(sdl_joy, s2ax);
		Sint16 yaxis2 = SDL_JoystickGetAxis(sdl_joy, s2ay);
		rightStick.x = double(xaxis2)/32768.0;
		rightStick.y = double(yaxis2)/32768.0;

		/*
		std::ostringstream os;
		os << "joy(" << position.x << ", " << position.y << ")";
		debugLog(os.str());
		*/


		callibrate(position, deadZone1);

		callibrate(rightStick, deadZone2);
		

		/*
		std::ostringstream os2;
		os2 << "joy2(" << position.x << ", " << position.y << ")";
		debugLog(os2.str());
		*/
		for (int i = 0; i < maxJoyBtns; i++)
			buttons[i] = SDL_JoystickGetButton(sdl_joy, i)?DOWN:UP;
		/*
		unsigned char btns[maxJoyBtns];
		glfwGetJoystickButtons(GLFW_JOYSTICK_1, btns, maxJoyBtns);
		for (int i = 0; i < maxJoyBtns; i++)
		{
			if (btns[i] == GLFW_PRESS)
				buttons[i] = DOWN;
			else
				buttons[i] = UP;
		}
		*/


	}
#endif

	if (clearRumbleTime > 0)
	{
		clearRumbleTime -= dt;
		if (clearRumbleTime < 0)
		{
			rumble(0,0,0);
		}
	}

#if defined(BBGE_BUILD_WINDOWS) && defined(BBGE_BUILD_XINPUT)
	if (inited && xinited)
	{
		XINPUT_STATE xinp;
		XInputGetState(0, &xinp);
		
		leftTrigger = float(xinp.Gamepad.bLeftTrigger)/255.0;
		rightTrigger = float(xinp.Gamepad.bRightTrigger)/255.0;

		leftShoulder = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
		rightShoulder = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;

		leftThumb = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
		rightThumb = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
		
		dpadUp = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
		dpadDown = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
		dpadLeft = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
		dpadRight = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

		
		

#if !defined(BBGE_BUILD_SDL)

		buttons[0] = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_A?DOWN:UP;
		buttons[1] = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_B?DOWN:UP;
		buttons[2] = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_X?DOWN:UP;
		buttons[3] = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_Y?DOWN:UP;

		position = Vector(xinp.Gamepad.sThumbLX, xinp.Gamepad.sThumbLY)/32768.0;
		position.y = -rightStick.y;

		rightStick = Vector(xinp.Gamepad.sThumbRX, xinp.Gamepad.sThumbRY)/32768.0;
		rightStick.y = -rightStick.y;

		callibrate(position, deadZone1);

		callibrate(rightStick, deadZone2);

#endif

		btnStart = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_START;
		btnSelect = xinp.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;
	}
#endif
		
		
		/*
		std::ostringstream os;
		os << "j-pos(" << position.x << ", " << position.y << " - b0[" << buttons[0] << "]) - len[" << len << "]";
		debugLog(os.str());
		*/
}

bool Joystick::anyButton()
{
	for (int i = 0; i < maxJoyBtns; i++)
	{
		if (buttons[i]) return true;
	}
	return false;
}
