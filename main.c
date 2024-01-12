#include <complex.h>
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

float pi;
float clamp(float min, float max, float val) {
	if (val > max)
		return max;
	if (val < min)
		return min;
	return val;
}

typedef struct {
	float left;
	float right;
} Frame;

#define FFT_SIZE 512
static unsigned int SAMPLE_RATE = 48000;
#define BASEBAND_RESOLUTION (SAMPLE_RATE/FFT_SIZE)
#define TIMEBLOCK_LENGTH (1/BASEBAND_RESOLUTION)


float global_raw_in[FFT_SIZE];
float complex global_raw_out[FFT_SIZE];

void FFT(float in[], size_t stride, float complex out[], size_t n) {
	assert(n>0);

	if (n == 1) {
		out[0] = (float complex) in[0];
		return;
	}

	FFT(in, stride * 2, out, n / 2);
	FFT(in + stride, stride * 2, out + n / 2, n / 2);

	for (size_t k = 0; k < n / 2; k++) {
		float t = (float)k / n;
		float complex v = cexp(-2 * I * pi * t) * out[k + n / 2];
		float complex e = out[k];
		out[k] = e + v;
		out[k + n / 2] = e - v;
	}
}

float Amplitude(float complex c) {
	float real = crealf(c);
	float img = cimagf(c);
	return sqrt(real*real + img*img);
}

float max_amp = 0.0f;

void ProcessAudio(void *buffer, unsigned int frames) {
	if (frames < FFT_SIZE)
		return;
	Frame *frame_data = (Frame *)buffer;

	for (size_t i = 0; i < frames; i++) {
		global_raw_in[i] = (frame_data[i].left + frame_data[i].right) / 2.0f;
	}
	FFT(global_raw_in, 1, global_raw_out, FFT_SIZE);

	max_amp = 0.0f;
	for (size_t i = 1; i < FFT_SIZE; i++) {
		float complex c = global_raw_out[i];
		float amp = Amplitude(c);
		if (max_amp < amp)
			max_amp = amp;
	}
}



#define VB_SIZE 10
static float VISUAL_BUFFER[VB_SIZE][VB_SIZE] = {0};

/* sample size if 512
 * variable sample rate should be handled
 * 0 * 48000 / 512 = 0Hz
 * 1 * 48000 / 512 = 93.75Hz
 * 2 * 48000 / 512 = 187.5Hz
 * 3 * 48000 / 512 = 281.25Hz
 * 4 * 48000 / 512 = 375Hz
 * 5 * 48000 / 512 = 468.75Hz
 * 6 * 48000 / 512 = 562.5Hz
 * 7 * 48000 / 512 = 656.25Hz
 * 8 * 48000 / 512 = 750Hz
 * 9 * 48000 / 512 = 843.75Hz
 * 10 * 48000 / 512 = 937.5Hz
 * 11 * 48000 / 512 = 1031.25Hz
 * 12 * 48000 / 512 = 1125Hz
 * and  so on.....
 * for bind < FFT_SIZE/2 
 *
 * should be enough.... i hope
 * yeah i give up
 */

void ProcessFFT(float dt){
	for (size_t i=0;i<VB_SIZE-1; i++){
		for (size_t j=0; j<FFT_SIZE;j++){
			VISUAL_BUFFER[i][j] = VISUAL_BUFFER[i+1][j];
		}
			
	}
	for(size_t i=1; i<FFT_SIZE; i++){
		float complex sample = global_raw_out[i];
		float amp = Amplitude(sample);
		amp = clamp(0.0f, 1.0f, amp/max_amp);
		VISUAL_BUFFER[VB_SIZE-1][i] = amp;
	}
}


void DrawFFT(float x, float y, float w, float h, Color color) {
	float bar_width = w / VB_SIZE;
	for (size_t i = 1; i < VB_SIZE; i++) {
		float height_mod = (i>VB_SIZE/2) ? 1+ ( 0.02*i) : 1+ (0.02*(VB_SIZE-i));
		float amp = 0.0f;
		for (size_t j=0; j<VB_SIZE; j++){
			amp += VISUAL_BUFFER[j][i];
		}
		amp /= (float) VB_SIZE;

		float bar_height = h * amp * height_mod;
		if (bar_height > h) bar_height=h;
		DrawRectangle(x + i * bar_width, y + h - bar_height, bar_width, bar_height, color);
	}
}

void DrawWaveForm(float x, float y, float w, float h, Color color){
	float bar_width = w / FFT_SIZE;
	float spacing = bar_width / 4;
	bar_width -= spacing;
	for (size_t i = 0; i < FFT_SIZE; i++) {
		float bar_height = h * global_raw_in[i];
		DrawRectangle(x + i * (bar_width + spacing), y + h/2 - bar_height/2,
			      bar_width, bar_height, color);
	}
	DrawLine(x, y + h/2, x + w, y + h/2, color);
}


int main(int argc, char **argv) {

	if (argc < 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	FILE *file = fopen(argv[1], "r");
	if (!file) {
		printf("file %s does not exist\n", argv[1]);
		return 1;
	}
	fclose(file);
	int screenWidth = 800;
	int screenHeight = 480;
	char *title = "Music Player";
	InitWindow(screenWidth, screenHeight, title);
	SetTargetFPS(60);

	pi = 4.0f * atan2f(1.0f, 1.0f);

	InitAudioDevice();
	Music music = LoadMusicStream(argv[1]);

	char *music_title = argv[1];
	music_title = strrchr(music_title, '/');
	if (music_title)
		music_title++;
	else
		music_title = argv[1];
	

	AttachAudioStreamProcessor(music.stream, ProcessAudio);
	PlayMusicStream(music);
	
	SAMPLE_RATE = music.stream.sampleRate;

	char playsound = 1;
	char drawfps = 0;
	char visual_mode = 1;

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();

		if (playsound) {
			UpdateMusicStream(music);
			ProcessFFT(dt);
		}

		if (IsKeyPressed(KEY_SPACE)) {
			playsound = !playsound;
			if (playsound)
				PlayMusicStream(music);
			else
				PauseMusicStream(music);
		}
		if (IsKeyPressed(KEY_RIGHT)) {
			float seek = GetMusicTimePlayed(music) + 5.0f;
			if (seek > GetMusicTimeLength(music))
				seek = GetMusicTimeLength(music);
			SeekMusicStream(music, seek);
		}
		if (IsKeyPressed(KEY_LEFT)) {
			float seek = GetMusicTimePlayed(music) - 5.0f;
			if (seek < 0.0f)
				seek = 0.0f;
			SeekMusicStream(music, seek);
		}
		if (GetMusicTimePlayed(music) >=
		    floor(GetMusicTimeLength(music))) {
			StopMusicStream(music);
			playsound = 0;
		}

		if (IsKeyPressed(KEY_F))
			drawfps = !drawfps;
		if (IsKeyPressed(KEY_V))
			visual_mode = !visual_mode;

		screenWidth = GetScreenWidth();
		screenHeight = GetScreenHeight();

		BeginDrawing();
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), RAYWHITE);
		DrawText(TextFormat("%s %s", (playsound) ? "Playing" : "Paused",
				    music_title),
			 20, 20, 20, LIGHTGRAY);
		char timer[20];
		sprintf(timer, "%02i:%02i/%02i:%02i",
			(int)GetMusicTimePlayed(music) / 60,
			(int)GetMusicTimePlayed(music) % 60,
			(int)GetMusicTimeLength(music) / 60,
			(int)GetMusicTimeLength(music) % 60);
		DrawText(timer, screenWidth - 150, 20, 20, LIGHTGRAY);

		DrawRectangle(20, 40, screenWidth - 40, 20, LIGHTGRAY);
		DrawRectangle(20, 40,
			      (screenWidth - 40) * GetMusicTimePlayed(music) /
				  GetMusicTimeLength(music),
			      20, MAROON);

		DrawRectangle(20, 70, screenWidth - 40, screenHeight - 200,
			      LIGHTGRAY);

		switch (visual_mode) {
			case 0:
				DrawWaveForm(20, 70, screenWidth - 40, screenHeight - 200, MAROON);
				break;
			case 1:
				DrawFFT(20, 70, screenWidth - 40, screenHeight - 200, MAROON);
				break;
			default:
				break;
		}

		DrawText("Space: Play/Pause | Left/Right: Back/Forward 5 secs",
			 20, screenHeight - 40, 20, LIGHTGRAY);

		if (drawfps) {
			DrawFPS(screenWidth - 100, screenHeight - 20);
		}
		EndDrawing();

	}
	UnloadMusicStream(music);
	CloseAudioDevice();
	CloseWindow();

	return 0;
}
