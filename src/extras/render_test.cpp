#include "common.h"

#if defined(REVC_RENDER_TESTS) && defined(LIBRW_GLFW)
#include "render_test.h"
#include "main.h"
#include "Frontend.h"
#include "Clock.h"
#include "Weather.h"
#include "PlayerPed.h"
#include "PlayerInfo.h"
#include "MBlur.h"
#include "postfx.h"
#include "custompipes.h"
#include "skeleton.h"
#include "crossplatform.h"
#include <algorithm>
#include <vector>
#include <cstdlib>

static bool active, gameTest, startedGame;
static int menuFrames, gameFrames, warmupFrames = 120, sampleFrames = 360;
static double previousFrame;
static std::vector<double> frameTimes;
static float travelDistance;
static CVector travelStart;

void
RenderTestInit(void)
{
	const char *mode = getenv("REVC_RENDER_TEST");
	if(!mode)
		return;
	if(strcmp(mode, "game") && strcmp(mode, "menu")){
		fprintf(stderr, "REVC_RENDER_TEST must be game or menu\n");
		exit(1);
	}
	active = true;
	gameTest = !strcmp(mode, "game");
	const char *frames = getenv("REVC_RENDER_TEST_FRAMES");
	if(frames)
		sampleFrames = std::max(1, atoi(frames));
	const char *travel = getenv("REVC_RENDER_TEST_TRAVEL");
	if(travel)
		travelDistance = atof(travel);
	frameTimes.reserve(sampleFrames);
	CMBlur::BlurOn = false;
#ifdef EXTENDED_COLOURFILTER
	CPostFX::EffectSwitch = CPostFX::POSTFX_OFF;
	CPostFX::BlurOn = CPostFX::MotionBlurOn = false;
#endif
#ifdef EXTENDED_PIPELINES
	CustomPipes::VehiclePipeSwitch = CustomPipes::VEHICLEPIPE_MATFX;
	CustomPipes::LightmapEnable = false;
	CustomPipes::GlossEnable = false;
	CustomPipes::RimlightEnable = false;
#endif
}

void
RenderTestFrame(RwCamera *camera)
{
	if(!active)
		return;
	if(gGameState == GS_FRONTEND){
		menuFrames++;
		if(gameTest){
			if(menuFrames >= 15 && !startedGame){
				startedGame = true;
				FrontEndMenuManager.DoSettingsBeforeStartingAGame();
			}
			return;
		}
		if(menuFrames < 60)
			return;
	}else if(gameTest && gGameState == GS_PLAYING_GAME){
		CClock::SetGameClock(22, 20);
		CWeather::ForceWeatherNow(WEATHER_SUNNY);
		gameFrames++;
		if(gameFrames == 1)
			travelStart = FindPlayerCoors();
		if(travelDistance != 0){
			float fraction = float(gameFrames) / (warmupFrames + sampleFrames);
			FindPlayerPed()->SetPosition(travelStart + CVector(0, travelDistance * fraction, 0));
		}
		double now = glfwGetTime();
		if(gameFrames > warmupFrames && previousFrame > 0)
			frameTimes.push_back((now - previousFrame) * 1000.0);
		previousFrame = now;
		if(int(frameTimes.size()) < sampleFrames)
			return;
	}else
		return;
	if(!frameTimes.empty()){
		double total = 0;
		for(double ms : frameTimes)
			total += ms;
		std::sort(frameTimes.begin(), frameTimes.end());
		int width, height;
		glfwGetFramebufferSize(PSGLOBAL(window), &width, &height);
		fprintf(stderr, "RENDER_TEST: %dx%d framebuffer, %zu frames, %.1f FPS, "
			"median %.2f ms, p95 %.2f ms, max %.2f ms\n", width, height,
			frameTimes.size(), 1000.0 * frameTimes.size() / total,
			frameTimes[frameTimes.size() / 2],
			frameTimes[(frameTimes.size() - 1) * 95 / 100], frameTimes.back());
	}
	const char *capture = getenv("REVC_RENDER_TEST_CAPTURE");
	if(capture){
		rw::Image *image = camera->frameBuffer->toImage();
		if(!image){
			fprintf(stderr, "RENDER_TEST: framebuffer readback failed\n");
			exit(1);
		}
		image->removeMask();
		rw::Error error;
		rw::getError(&error);
		rw::writePNG(image, capture);
		image->destroy();
		rw::getError(&error);
		if(error.code){
			fprintf(stderr, "RENDER_TEST: could not write capture %s\n", capture);
			exit(1);
		}
	}
	fprintf(stderr, "RENDER_TEST: completed %s\n", gameTest ? "game" : "menu");
	active = false;
	RsGlobal.quit = TRUE;
}
#endif
