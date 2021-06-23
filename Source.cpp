/*This source code copyrighted by Lazy Foo' Productions (2004-2020)
and may not be redistributed without written permission.*/

//Using SDL, SDL_image, standard IO, and strings
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string>
#include <cmath>
#include <sstream>

//Screen dimension constants
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 760;
double angle = 0;
bool left = 0, right = 0, up = 0, down = 0;
int offsetX = 0, offsetY = 0, counter=0;

//Texture wrapper class
class LTexture
{
public:
	//Initializes variables
	LTexture();

	//Deallocates memory
	~LTexture();

	//Loads image at specified path
	bool loadFromFile(std::string path);

#if defined(SDL_TTF_MAJOR_VERSION)
	//Creates image from font string
	bool loadFromRenderedText(std::string textureText, SDL_Color textColor);
#endif

	//Deallocates texture
	void free();

	//Set color modulation
	void setColor(Uint8 red, Uint8 green, Uint8 blue);

	//Set blending
	void setBlendMode(SDL_BlendMode blending);

	//Set alpha modulation
	void setAlpha(Uint8 alpha);

	//Renders texture at given point
	void render(int x, int y, SDL_Rect* clip = NULL, double angle = NULL, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE);

	//Gets image dimensions
	int getWidth();
	int getHeight();

private:
	//The actual hardware texture
	SDL_Texture* mTexture;

	//Image dimensions
	int mWidth;
	int mHeight;
};

//The application time based timer
class LTimer
{
public:
	//Initializes variables
	LTimer();

	//The various clock actions
	void start();
	void stop();
	void pause();
	void unpause();

	//Gets the timer's time
	Uint32 getTicks();

	//Checks the status of the timer
	bool isStarted();
	bool isPaused();

private:
	//The clock time when the timer started
	Uint32 mStartTicks;

	//The ticks stored when the timer was paused
	Uint32 mPausedTicks;

	//The timer status
	bool mPaused;
	bool mStarted;
};

//The car that will move around on the screen
class Car
{
public:
	//The dimensions of the car
	static const int CAR_WIDTH = 128;
	static const int CAR_HEIGHT = 64;

	//Maximum axis velocity of the car
	static const int CAR_VEL = 50;

	//Initializes the variables
	Car();

	//Takes key presses and adjusts the car's velocity
	void handleEvent(SDL_Event& e);

	//Moves the car
	void move(int wall);

	//Shows the car on the screen
	void render();

	//The X and Y offsets of the car
	float mPosX, mPosY;

	//The velocity of the car
	float mVel;

	float acc, dec, turnSpeed;

	SDL_Rect mCollider;
};

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();

//Box collision detector
bool checkCollision(SDL_Rect a, SDL_Rect b);

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

//Scene textures
LTexture gCarTexture;
LTexture background;
LTexture gTimeTextTexture;

LTexture::LTexture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}

bool LTexture::loadFromFile(std::string path)
{
	//Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL)
	{
		printf("Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError());
	}
	else
	{
		//Color key image
		SDL_SetColorKey(loadedSurface, SDL_TRUE, SDL_MapRGB(loadedSurface->format, 0, 0xFF, 0xFF));

		//Create texture from surface pixels
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (newTexture == NULL)
		{
			printf("Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface(loadedSurface);
	}

	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}

#if defined(SDL_TTF_MAJOR_VERSION)
bool LTexture::loadFromRenderedText(std::string textureText, SDL_Color textColor)
{
	//Get rid of preexisting texture
	free();

	//Render text surface
	SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, textureText.c_str(), textColor);
	if (textSurface != NULL)
	{
		//Create texture from surface pixels
		mTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
		if (mTexture == NULL)
		{
			printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
		}
		else
		{
			//Get image dimensions
			mWidth = textSurface->w;
			mHeight = textSurface->h;
		}

		//Get rid of old surface
		SDL_FreeSurface(textSurface);
	}
	else
	{
		printf("Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError());
	}


	//Return success
	return mTexture != NULL;
}
#endif

void LTexture::free()
{
	//Free texture if it exists
	if (mTexture != NULL)
	{
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::setColor(Uint8 red, Uint8 green, Uint8 blue)
{
	//Modulate texture rgb
	SDL_SetTextureColorMod(mTexture, red, green, blue);
}

void LTexture::setBlendMode(SDL_BlendMode blending)
{
	//Set blending function
	SDL_SetTextureBlendMode(mTexture, blending);
}

void LTexture::setAlpha(Uint8 alpha)
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod(mTexture, alpha);
}

void LTexture::render(int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip)
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };

	//Set clip rendering dimensions
	if (clip != NULL)
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx(gRenderer, mTexture, clip, &renderQuad, angle, center, flip);
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}

LTimer::LTimer()
{
	//Initialize the variables
	mStartTicks = 0;
	mPausedTicks = 0;

	mPaused = false;
	mStarted = false;
}

void LTimer::start()
{
	//Start the timer
	mStarted = true;

	//Unpause the timer
	mPaused = false;

	//Get the current clock time
	mStartTicks = SDL_GetTicks();
	mPausedTicks = 0;
}

void LTimer::stop()
{
	//Stop the timer
	mStarted = false;

	//Unpause the timer
	mPaused = false;

	//Clear tick variables
	mStartTicks = 0;
	mPausedTicks = 0;
}

void LTimer::pause()
{
	//If the timer is running and isn't already paused
	if (mStarted && !mPaused)
	{
		//Pause the timer
		mPaused = true;

		//Calculate the paused ticks
		mPausedTicks = SDL_GetTicks() - mStartTicks;
		mStartTicks = 0;
	}
}

void LTimer::unpause()
{
	//If the timer is running and paused
	if (mStarted && mPaused)
	{
		//Unpause the timer
		mPaused = false;

		//Reset the starting ticks
		mStartTicks = SDL_GetTicks() - mPausedTicks;

		//Reset the paused ticks
		mPausedTicks = 0;
	}
}

Uint32 LTimer::getTicks()
{
	//The actual timer time
	Uint32 time = 0;

	//If the timer is running
	if (mStarted)
	{
		//If the timer is paused
		if (mPaused)
		{
			//Return the number of ticks when the timer was paused
			time = mPausedTicks;
		}
		else
		{
			//Return the current time minus the start time
			time = SDL_GetTicks() - mStartTicks;
		}
	}

	return time;
}

bool LTimer::isStarted()
{
	//Timer is running and paused or unpaused
	return mStarted;
}

bool LTimer::isPaused()
{
	//Timer is running and paused
	return mPaused && mStarted;
}

bool checkCollision(SDL_Rect a, SDL_Rect b)
{
	//The sides of the rectangles
	int leftA, leftB;
	int rightA, rightB;
	int topA, topB;
	int bottomA, bottomB;

	//Calculate the sides of rect A
	leftA = a.x;
	rightA = a.x + a.w;
	topA = a.y;
	bottomA = a.y + a.h;

	//Calculate the sides of rect B
	leftB = b.x;
	rightB = b.x + b.w;
	topB = b.y;
	bottomB = b.y + b.h;

	//If any of the sides from A are outside of B
	if (bottomA <= topB)
	{
		return false;
	}

	if (topA >= bottomB)
	{
		return false;
	}

	if (rightA <= leftB)
	{
		return false;
	}

	if (leftA >= rightB)
	{
		return false;
	}

	//If none of the sides from A are outside B
	return true;
}

Car::Car()
{
	//Initialize the offsets
	mPosX = 1384;
	mPosY = 580;

	//Set collision box dimension
	mCollider.w = CAR_WIDTH;
	mCollider.h = CAR_HEIGHT;

	//Initialize the velocity
	mVel = 0;

	acc = 0.1;
	dec = 0.2;

	turnSpeed = 0.08;
}

void Car::handleEvent(SDL_Event& e)
{	
	const Uint8* keystate = SDL_GetKeyboardState(NULL);

	
	//If a key was pressed
	if (e.type == SDL_KEYDOWN && e.key.repeat == 0)
	{
		switch (e.key.keysym.sym)
		{
		case SDLK_UP: up = 1; break;
		case SDLK_DOWN: down = 1; break;
		case SDLK_RIGHT: right = 1; break;
		case SDLK_LEFT: left = 1; break;
		}
	}
	//If a key was released
	if (e.type == SDL_KEYUP && e.key.repeat == 0)
	{
		switch (e.key.keysym.sym)
		{
		case SDLK_UP: up = 0; break;
		case SDLK_DOWN: down = 0; break;
		case SDLK_RIGHT: right = 0; break;
		case SDLK_LEFT: left = 0; break;
		}
	}
	//Handle input for the car
}

void Car::move(int wall)
{
	if (up && mVel < CAR_VEL) 
		if (mVel < 0) mVel += dec;
		else mVel += acc;

	if (down && mVel > -CAR_VEL)
		if (mVel > 0) mVel -= dec;
		else mVel -= acc;

	if (!up && !down)
		if (mVel - dec > 0) mVel -= dec;
		else if (mVel + dec < 0) mVel += dec;
		else mVel = 0;

	if (right && mVel != 0) angle += turnSpeed * mVel / CAR_VEL;
	if (left && mVel != 0) angle -= turnSpeed * mVel / CAR_VEL;

	switch (wall)
	{
	case 0: {
		mPosX += cos(angle) * mVel;
		mPosY += sin(angle) * mVel; 
		break; //normal
	}
	case 1: {
		mPosX = 1;
		mPosY += sin(angle) * mVel;
		break; //left
	}
	case 2: {
		mPosX = 3871;
		mPosY += sin(angle) * mVel;
		break; //right
	}
	case 3: {
		mPosX += cos(angle) * mVel;
		mPosY = 1;
		break; //up
	}
	case 4: {
		mPosX += cos(angle) * mVel;
		mPosY = 1921;
		break; //down
	}
	}

	mCollider.x = mPosX;
	mCollider.y = mPosY;

	if (mPosX > 640 && mPosX < 3360) offsetX = mPosX - 640;
	if (mPosY > 380 && mPosY < 1620) offsetY = mPosY - 380;

	//const int SCREEN_WIDTH = 1280;
	//const int SCREEN_HEIGHT = 760;

	//printf("%f %f \n",mPosX,mPosY);

}

void Car::render()
{
	//Show the car
	gCarTexture.render(mPosX, mPosY);
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			printf("Warning: Linear texture filtering not enabled!");
		}

		//Create window
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create vsynced renderer for window
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (gRenderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags))
				{
					printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
					success = false;
				}
			}
		}
	}

	return success;
}

bool loadMedia()
{
	//Loading success flag
	bool success = true;

	//Load car texture
	if (!gCarTexture.loadFromFile("C:/Users/19afr/Documents/PJATK/SEM6/SGD/car.bmp"))
	{
		printf("Failed to load car texture!\n");
		success = false;
	}
	if (!background.loadFromFile("C:/Users/19afr/Documents/PJATK/SEM6/SGD/background3.png"))
	{
		printf("Failed to load background texture!\n");
		success = false;
	}

	return success;
}

void close()
{
	//Free loaded images
	gCarTexture.free();
	background.free();
	gTimeTextTexture.free();

	//Destroy window	
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

int main(int argc, char* args[])
{
	//Start up SDL and create window
	if (!init())
	{
		printf("Failed to initialize!\n");
	}
	else
	{
		//Load media
		if (!loadMedia())
		{
			printf("Failed to load media!\n");
		}
		else
		{
			//Main loop flag
			bool quit = false;

			//Event handler
			SDL_Event e;

			//The application timer
			LTimer timer;

			//The car that will be moving around on the screen
			Car car;

			//Set the walls
#pragma region walls
			SDL_Rect wallL;
			wallL.x = -40;
			wallL.y = -40;
			wallL.w = 40;
			wallL.h = 2080;

			SDL_Rect wallR;
			wallR.x = 4000;
			wallR.y = -40;
			wallR.w = 40;
			wallR.h = 2080;

			SDL_Rect wallU;
			wallU.x = -40;
			wallU.y = -40;
			wallU.w = 4080;
			wallU.h = 40;

			SDL_Rect wallD;
			wallD.x = -40;
			wallD.y = 2000;
			wallD.w = 4080;
			wallD.h = 40;

			SDL_Rect check1;
			check1.x = 2371;
			check1.y = 229;
			check1.w = 40;
			check1.h = 200;

			SDL_Rect check2;
			check2.x = 3282;
			check2.y = 90;
			check2.w = 40;
			check2.h = 200;

			SDL_Rect check3;
			check3.x = 3425;
			check3.y = 508;
			check3.w = 40;
			check3.h = 200;

			SDL_Rect check4;
			check4.x = 2960;
			check4.y = 962;
			check4.w = 200;
			check4.h = 40;

			SDL_Rect check5;
			check5.x = 1980;
			check5.y = 838;
			check5.w = 40;
			check5.h = 200;

			SDL_Rect check6;
			check6.x = 1880;
			check6.y = 1440;
			check6.w = 200;
			check6.h = 40;

			SDL_Rect check7;
			check7.x = 760;
			check7.y = 1150;
			check7.w = 40;
			check7.h = 200;

			SDL_Rect check8;
			check8.x = 720;
			check8.y = 720;
			check8.w = 40;
			check8.h = 200;

			SDL_Rect check9;
			check9.x = 1500;
			check9.y = 500;
			check9.w = 40;
			check9.h = 200;
#pragma endregion walls

			SDL_Rect checkpoints[9] = { check1,check2,check3,check4,check5,check6,check7,check8,check9 };

			//While application is running
			while (!quit)
			{	//Handle events on queue
				while (SDL_PollEvent(&e) != 0)
				{
					//User requests quit
					if (e.type == SDL_QUIT)
					{
						quit = true;
					}
					//Handle input for the car
					car.handleEvent(e);
				}

				
				if (checkCollision(wallL, car.mCollider)) {
					car.move(1);
				}
				else if (checkCollision(wallR, car.mCollider)) {
					car.move(2);
				}
				else if (checkCollision(wallU, car.mCollider)) {
					car.move(3);
				}
				else if (checkCollision(wallD, car.mCollider)) {
					car.move(4);
				}
				else {
					car.move(0);
				}
				if (checkCollision(car.mCollider,checkpoints[counter])) {
					counter++;
					if (counter == 1) {
						timer.start();
					}
					else if (counter == 9) {
						timer.pause();
					}
					printf("%i %f \n",counter,timer.getTicks() / 1000.f);
				}

				//Clear screen
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
				SDL_RenderClear(gRenderer);

				//Render objects
				background.render(-offsetX, -offsetY);
				gCarTexture.render(car.mPosX - offsetX, car.mPosY - offsetY, 0, angle * 180 / M_PI);

				//Update screen
				SDL_RenderPresent(gRenderer);
			}
		}
	}

	//Free resources and close SDL
	close();

	return 0;
}