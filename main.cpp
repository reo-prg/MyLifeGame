#include <DxLib.h>
#include <string>
#include <vector>
#include <functional>
#include <random>

#define WIDTH 80
#define HEIGHT 70
#define GRID_SIZE 8

#define SCRWIDTH (WIDTH * GRID_SIZE + 1)
#define SCRHEIGHT (HEIGHT * GRID_SIZE + 1)

#define WNDSIZE 720
#define GRID_OFFSET_X ((WNDSIZE - SCRWIDTH)/ 2)
#define GRID_OFFSET_Y ((WNDSIZE - SCRHEIGHT)/ 2)

#define SPEED_STEP 4

#define SPEED_OFFSETX 400
#define SPEED_OFFSETY 20
#define SPEED_SIZEX 50
#define SPEED_SIZEY 30
#define SPEED_INT 4

#define BUTTON_SIZEX 70
#define BUTTON_SIZEY 30

#define DEF_DURATION 40
#define SCR_OFFSETY 10

enum class InputType
{
	NON,
	OFF,
	ON
};

enum class ButtonState
{
	NON,
	OVER,
	ACTIVE,
	NONACTIVE
};

class BoxButton
{
public:
	BoxButton()
	{
		px_ = py_ = sx_ = sy_ = 0;
		color_[0] = 0x00ff00;
		color_[1] = 0x00ff00;
		color_[2] = 0x00ff00;
		color_[3] = 0x00ff00;
		fill_ = true;
		state_ = ButtonState::NON;
	}
	void SetBox(int pos_x, int pos_y, int size_x, int size_y,
		unsigned int color0, unsigned int color1, unsigned int color2, unsigned int color3, bool fill)
	{
		px_ = pos_x;
		py_ = pos_y;
		sx_ = size_x;
		sy_ = size_y;
		color_[0] = color0;
		color_[1] = color1;
		color_[2] = color2;
		color_[3] = color3;
		fill_ = fill;
		drawbox_ = true;
	}

	void SetDrawString(const std::string& str, unsigned int str_color)
	{
		str_ = str;
		strColor_ = str_color;
		drawstr_ = true;
	}

	void Draw(void)
	{
		if (!drawbox_)
		{
			return;
		}
		DrawBox(px_, py_, px_ + sx_, py_ + sy_, color_[static_cast<int>(state_)], fill_);
		if (drawstr_)
		{
			int l = GetDrawStringWidth(str_.c_str(), strlen2Dx(str_.c_str()));
			DrawString(px_ + sx_ / 2 - l / 2, py_ + sy_ / 2 - 10, str_.c_str(), strColor_);
		}
	}

	void IsHit(int x, int y, bool cur, bool prev)
	{
		if (state_ == ButtonState::NONACTIVE) { return; }

		if (x > px_ && x < (px_ + sx_) &&
			y > py_ && y < (py_ + sy_))
		{
			if (state_ == ButtonState::ACTIVE && !cur && prev)
			{
				func_();
				return;
			}
			state_ = ButtonState::OVER;
			if (cur)
			{
				state_ = ButtonState::ACTIVE;
				return;
			}
			return;
		}
		state_ = ButtonState::NON;
		return;
	}

	void Activate(void)
	{
		func_();
	}

	std::function<void(void)> func_ = [] {};
	ButtonState state_;
private:
	int px_, py_, sx_, sy_;
	unsigned int color_[4];
	bool fill_;
	std::string str_;
	unsigned int strColor_;

	bool drawbox_ = false;
	bool drawstr_ = false;
};

void Stop(int mx, int my, bool cur, bool prev);
void ExecuteLifes(int mx, int my, bool cur, bool prev);

bool lifes_[2][HEIGHT][WIDTH];
size_t currentBuffer = 0;
BoxButton start;
BoxButton stop;
BoxButton reset;
BoxButton random;
BoxButton speed[SPEED_STEP];
void (*currentUpdate)(int, int, bool, bool);
InputType currentType = InputType::NON;
constexpr int speedList[SPEED_STEP] = { 1, 2, 4, 8 };
std::mt19937 mt;
std::uniform_int_distribution<int> ran(0, 1);

int counter = 0;
int gen = 0;
int currentSpeed = 1;

int _stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	SetOutApplicationLogValidFlag(false);
	ChangeWindowMode(true);
	SetGraphMode(WNDSIZE, WNDSIZE, 32);
	SetMainWindowText(_T("LifeGame"));
	SetAlwaysRunFlag(true);
	SetDoubleStartValidFlag(true);
	if (DxLib_Init() == -1)
	{
		return -1;
	}

	int scr = MakeScreen(SCRWIDTH, SCRHEIGHT, true);
	SetDrawScreen(scr);
	ClsDrawScreen();
	for (int i = 0; i <= WIDTH; i++)
	{
		DrawLine(i * GRID_SIZE, 0, i * GRID_SIZE, SCRHEIGHT - 1, 0xffffff);
		DrawLine(0, i * GRID_SIZE, SCRWIDTH - 1, i * GRID_SIZE, 0xffffff);
	}
	SetDrawScreen(DX_SCREEN_BACK);

	std::random_device rd;
	mt = std::mt19937(rd());

	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			lifes_[0][i][j] = false;
			lifes_[1][i][j] = false;
		}
	}

	int mouseX, mouseY;
	bool prevMouseLeft = true;

	start = BoxButton();
	start.SetBox(100, WNDSIZE - 50, BUTTON_SIZEX, BUTTON_SIZEY, 0x66ff66, 0x44dd44, 0x228822, 0x222222, true);
	start.SetDrawString("Start", 0x000000);
	start.func_ = []()
	{
		currentUpdate = ExecuteLifes;
		start.state_ = ButtonState::NONACTIVE;
		stop.state_ = ButtonState::NON;
		reset.state_ = ButtonState::NONACTIVE;
		random.state_ = ButtonState::NONACTIVE;
		counter = 0;
	};
	start.state_ = ButtonState::NON;

	stop = BoxButton();
	stop.SetBox((WNDSIZE - 250) / 3 + 100, WNDSIZE - 50, BUTTON_SIZEX, BUTTON_SIZEY, 0xff5566, 0xdd4444, 0x882222, 0x222222, true);
	stop.SetDrawString("Stop", 0x000000);
	stop.func_ = []()
	{
		currentUpdate = Stop;
		start.state_ = ButtonState::NON;
		stop.state_ = ButtonState::NONACTIVE;
		reset.state_ = ButtonState::NON;
		random.state_ = ButtonState::NON;
	};
	stop.state_ = ButtonState::NONACTIVE;

	reset = BoxButton();
	reset.SetBox((WNDSIZE - 250) * 2 / 3 + 100, WNDSIZE - 50, BUTTON_SIZEX, BUTTON_SIZEY, 0x6666ff, 0x4444dd, 0x222288, 0x000044, true);
	reset.SetDrawString("Reset", 0x000000);
	reset.func_ = []()
	{
		for (int i = 0; i < HEIGHT; i++)
		{
			for (int j = 0; j < WIDTH; j++)
			{
				lifes_[0][i][j] = false;
				lifes_[1][i][j] = false;
			}
		}
	};
	reset.state_ = ButtonState::NON;

	random = BoxButton();
	random.SetBox(WNDSIZE - 150, WNDSIZE - 50, BUTTON_SIZEX, BUTTON_SIZEY, 0x6666ff, 0x4444dd, 0x222288, 0x000044, true);
	random.SetDrawString("Random", 0x000000);
	random.func_ = []()
	{
		for (int i = 0; i < HEIGHT; i++)
		{
			for (int j = 0; j < WIDTH; j++)
			{
				bool l = (ran(mt) == 0);
				lifes_[0][i][j] = l;
				lifes_[1][i][j] = l;
			}
		}
	};
	random.state_ = ButtonState::NON;

	for (int i = 0; i < SPEED_STEP; i++)
	{
		speed[i] = BoxButton();
		speed[i].SetBox(SPEED_OFFSETX + (SPEED_SIZEX + SPEED_INT) * i, SPEED_OFFSETY, 
			SPEED_SIZEX, SPEED_SIZEY,
			0xffffff, 0xbbbbbb, 0x888888, 0x444444, true);
		speed[i].SetDrawString(("x" + std::to_string(speedList[i])).c_str(), 0x000000);
		speed[i].func_ = [i]() 
		{
			currentSpeed = speedList[i];
		};
		speed[i].state_ = ButtonState::NON;
	}


	currentUpdate = Stop;

	while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
	{
		GetMousePoint(&mouseX, &mouseY);
		int mState = GetMouseInput();
		bool currentLeft = (mState & MOUSE_INPUT_LEFT);

		currentUpdate(mouseX, mouseY, currentLeft, prevMouseLeft);

		start.IsHit(mouseX, mouseY, currentLeft, prevMouseLeft);
		stop.IsHit(mouseX, mouseY, currentLeft, prevMouseLeft);
		reset.IsHit(mouseX, mouseY, currentLeft, prevMouseLeft);
		random.IsHit(mouseX, mouseY, currentLeft, prevMouseLeft);
		for (int i = 0; i < SPEED_STEP; i++)
		{
			speed[i].IsHit(mouseX, mouseY, currentLeft, prevMouseLeft);
		}

		// 描画
		ClsDrawScreen();

		DrawGraph((WNDSIZE - SCRWIDTH) / 2, (WNDSIZE - SCRHEIGHT) / 2 + SCR_OFFSETY, scr, true);
		start.Draw();
		stop.Draw();
		reset.Draw();
		random.Draw();
		for (int i = 0; i < SPEED_STEP; i++)
		{
			speed[i].Draw();
		}

		for (int i = 0; i < HEIGHT; i++)
		{
			for (int j = 0; j < WIDTH; j++)
			{
				if (lifes_[currentBuffer][i][j])
				{
					DrawBox(GRID_OFFSET_X + j * GRID_SIZE + 1, GRID_OFFSET_Y + i * GRID_SIZE + 1 + SCR_OFFSETY,
						GRID_OFFSET_X + (j + 1) * GRID_SIZE - 1, GRID_OFFSET_Y + (i + 1) * GRID_SIZE - 1 + SCR_OFFSETY, 0x66ff66, true);
				}
			}
		}
		DrawFormatString(10, 10, 0xffffff, "%d世代", gen);
		DrawString(340, 30, "SPEED:", 0xffffff);
		DrawFormatString(450, 70, 0xffffff, "Current Speed:x%d", currentSpeed);

		DrawBox(335, 15, SPEED_OFFSETX + (SPEED_SIZEX + SPEED_INT) * SPEED_STEP, 
			SPEED_OFFSETY + SPEED_SIZEY + 5, 0xffffff, false);
		ScreenFlip();

		prevMouseLeft = currentLeft;
	}


	DxLib_End();
	return 0;
}

void Stop(int mx, int my, bool cur, bool prev)
{
	if (cur)
	{
		if (mx >= GRID_OFFSET_X && mx < GRID_OFFSET_X + SCRWIDTH - 1 &&
			my >= GRID_OFFSET_Y && my < GRID_OFFSET_Y + SCRHEIGHT - 1)
		{
			bool& c = lifes_[currentBuffer][(my - GRID_OFFSET_Y) / GRID_SIZE][(mx - GRID_OFFSET_X) / GRID_SIZE];
			switch (currentType)
			{
			case InputType::NON:
				if (!prev)
				{
					currentType = (c ? InputType::OFF : InputType::ON);
					c = !c;
					gen = 0;
				}
				break;
			case InputType::OFF:
				c = false;
				break;
			case InputType::ON:
				c = true;
				break;
			default:
				break;
			}
		}
	}
	else
	{
		currentType = InputType::NON;
	}
}

void ExecuteLifes(int mx, int my, bool cur, bool prev)
{
	counter++;
	if (counter < (DEF_DURATION / currentSpeed)) { return; }
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			lifes_[1 - currentBuffer][y][x] = false;
			int lc = 0;
			for (int i = -1; i <= 1; i++)
			{
				for (int j = -1; j <= 1; j++)
				{
					if (i == 0 && j == 0) { continue; }
					if (x + j >= 0 && x + j < WIDTH && y + i >= 0 && y + i < HEIGHT)
					{
						if (lifes_[currentBuffer][y + i][x + j]) { lc++; }
					}
				}
			}

			if (lifes_[currentBuffer][y][x])
			{
				lifes_[1 - currentBuffer][y][x] = (lc == 2 || lc == 3);
			}
			else
			{
				lifes_[1 - currentBuffer][y][x] = (lc == 3);
			}
		}
	}
	currentBuffer = 1 - currentBuffer;
	counter = 0;
	gen++;
}