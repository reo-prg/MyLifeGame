#include <DxLib.h>
#include <string>
#include <vector>
#include <functional>

#define WIDTH 80
#define HEIGHT 70
#define GRID_SIZE 8

#define SCRWIDTH (WIDTH * GRID_SIZE + 1)
#define SCRHEIGHT (HEIGHT * GRID_SIZE + 1)

#define WNDSIZE 720
#define GRID_OFFSET_X ((WNDSIZE - SCRWIDTH)/ 2)
#define GRID_OFFSET_Y ((WNDSIZE - SCRHEIGHT)/ 2)

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

	void SetDrawString(const std::wstring& str, unsigned int str_color)
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
			DrawString(px_ + sx_ / 2 - l / 2, py_ + sy_ / 2, str_.c_str(), strColor_);
		}
	}

	void IsHit(int x, int y, bool cur, bool prev)
	{
		if (state_ == ButtonState::NONACTIVE) { return; }

		if (x > px_ && x < (px_ + sx_) &&
			y > py_ && y < (py_ + sy_))
		{
			if (state_ == ButtonState::ACTIVE && !cur)
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
	std::wstring str_;
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
void (*currentUpdate)(int, int, bool, bool);
InputType currentType = InputType::NON;

int counter = 0;
int gen = 0;

int _stdcall WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	SetOutApplicationLogValidFlag(false);
	ChangeWindowMode(true);
	SetGraphMode(WNDSIZE, WNDSIZE, 32);
	SetMainWindowText(_T("LifeGame"));
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
	start.SetBox(100, WNDSIZE - 50, 50, 30, 0x66ff66, 0x44dd44, 0x228822, 0x222222, true);
	start.SetDrawString(L"Start", 0x000000);
	start.func_ = []() 
	{
		currentUpdate = ExecuteLifes;
		start.state_ = ButtonState::NONACTIVE;
		stop.state_ = ButtonState::NON;
		reset.state_ = ButtonState::NONACTIVE;
		counter = 0;
	};
	start.state_ = ButtonState::NON;

	stop = BoxButton();
	stop.SetBox((WNDSIZE - 50) / 2, WNDSIZE - 50, 50, 30, 0xff5566, 0xdd4444, 0x882222, 0x222222, true);
	stop.SetDrawString(L"Stop", 0x000000);
	stop.func_ = []()
	{
		currentUpdate = Stop;
		start.state_ = ButtonState::NON;
		stop.state_ = ButtonState::NONACTIVE;
		reset.state_ = ButtonState::NON;
	};
	stop.state_ = ButtonState::NONACTIVE;

	reset = BoxButton();
	reset.SetBox(WNDSIZE - 150, WNDSIZE - 50, 50, 30, 0x66ff66, 0x44dd44, 0x228822, 0x222222, true);
	reset.SetDrawString(L"Reset", 0x000000);
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

		// •`‰æ
		ClsDrawScreen();
		
		DrawGraph((WNDSIZE - SCRWIDTH) / 2, (WNDSIZE - SCRHEIGHT) / 2, scr, true);
		start.Draw();
		stop.Draw();
		reset.Draw();

		for (int i = 0; i < HEIGHT; i++)
		{
			for (int j = 0; j < WIDTH; j++)
			{
				if (lifes_[currentBuffer][i][j])
				{
					DrawBox(GRID_OFFSET_X + j * GRID_SIZE + 1, GRID_OFFSET_Y + i * GRID_SIZE + 1,
						GRID_OFFSET_X + (j + 1) * GRID_SIZE - 1, GRID_OFFSET_Y + (i + 1) * GRID_SIZE - 1, 0x66ff66, true);
				}
			}
		}
		DrawFormatString(10, 10, 0xffffff, L"%d¢‘ã", gen);
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
	if (counter < 40) { return; }
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
