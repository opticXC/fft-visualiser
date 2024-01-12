#include <bits/stdint-uintn.h>
#include <complex.h>
#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

float global_raw_in[FFT_SIZE];
float complex global_raw_out[FFT_SIZE];

void FFT(float in[], size_t stride, float complex out[], size_t n) {
	if (n < 0)
		return;

	if (n == 1) {
		out[0] = in[0];
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
	return logf(real*real + img*img);
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
	for (size_t i = 0; i < FFT_SIZE; i++) {
		float complex c = global_raw_out[i];
		float amp = Amplitude(c);
		if (max_amp < amp)
			max_amp = amp;
	}
}



unsigned int FFT_FREQS[12] = {16, 32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000, 22000};
#define VB_SIZE 12
static float VISUAL_BUFFER[VB_SIZE] = {0.0f};

/* sample size if 512
 * variable sample rate should be handled
 * 0 * 44100 / 512 = 0Hz
 * 1 * 44100 / 512 = 43.1Hz
 * 2 * 44100 / 512 = 86.1Hz
 * 3 * 44100 / 512 = 129.2Hz
 * 4 * 44100 / 512 = 172.3Hz
 * 5 * 44100 / 512 = 215.3Hz
 * 6 * 44100 / 512 = 258.4Hz
 * 7 * 44100 / 512 = 301.5Hz
 * 8 * 44100 / 512 = 344.6Hz
 * 9 * 44100 / 512 = 387.6Hz
 * 10 * 44100 / 512 = 430.7Hz
 * and  so on.....
 * 
 * for the needed freq, we need to find the closest sample
 * 32Hz = 0.73 * 43.1Hz
 * 64Hz = 1.47 * 43.1Hz or 0.73 * 86.1Hz
 * 125Hz = 1.47 * 86.1Hz or 0.73 * 172.3Hz
 * 250Hz = 1.47 * 172.3Hz or 0.73 * 344.6Hz
 * 500Hz = 1.47 * 344.6Hz or 0.73 * 689.1Hz
 * 1000Hz = 1.47 * 689.1Hz or 0.73 * 1378.1Hz
 * 2000Hz = 1.47 * 1378.1Hz or 0.73 * 2756.3Hz
 * 4000Hz = 1.47 * 2756.3Hz or 0.73 * 5512.5Hz
 * 8000Hz = 1.47 * 5512.5Hz or 0.73 * 11025Hz
 * 16000Hz = 1.47 * 11025Hz or 0.73 * 22050Hz
 * should be enough.... i hope
 *
 * 0.73 should be more consistent...
 */

unsigned int sample_rate = 44100;

void ProcessFFT(){
	for (size_t i=0; i<VB_SIZE; i++){
		unsigned int freq = FFT_FREQS[i];
		unsigned int sample = freq * 44100 / FFT_SIZE;
		unsigned int sample_index = sample / (sample_rate / FFT_SIZE);
		float complex c = global_raw_out[sample_index];
		float amp = Amplitude(c);
		VISUAL_BUFFER[i] = amp / max_amp;
	}
}

void PrintSamples(){
	for (size_t i=0; i<VB_SIZE; i++){
		unsigned int freq = FFT_FREQS[i];
		float amp = VISUAL_BUFFER[i];
		printf("%iHz: %f\n", freq, amp);
	}
}

void DrawFFT(float x, float y, float w, float h, Color color) {
	float bar_width = w / VB_SIZE;
	float spacing = bar_width / 4;
	bar_width -= spacing;
	for (size_t i = 0; i < VB_SIZE; i++) {
		float bar_height = h * VISUAL_BUFFER[i];
		DrawRectangle(x + i * (bar_width + spacing), y + h - bar_height,
			      bar_width, bar_height, color);
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
	sample_rate = music.stream.sampleRate;
	char playsound = 1;
	char drawfps = 0;

	char visual_mode = 0;

	while (!WindowShouldClose()) {
		if (playsound) {
			UpdateMusicStream(music);
			ProcessFFT();
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
		ClearBackground(RAYWHITE);
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
