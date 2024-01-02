#include <bits/stdint-uintn.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <complex.h>
#include <assert.h>




float pi;
float clamp(float min, float max, float val){
    if (val > max) return max;
    if (val < min) return min;
    return val;
}


typedef struct{
    float left;
    float right;
} Frame;

#define FFT_SIZE 256

float global_raw_in[FFT_SIZE];
float complex global_raw_out[FFT_SIZE];

void FFT(float in[], size_t stride, float complex out[], size_t n){
    assert(n>0);
    if (n == 1){
	out[0] = in[0];
	return;
    }
    
    FFT(in, stride*2, out, n/2);
    FFT(in + stride, stride*2, out + n/2, n/2);

    for (size_t k=0; k<n/2; k++){
	float t = (float)k/n;
	float complex v = cexp(-2 * I* pi * t) * out[k + n/2];
	float complex e = out[k];
	out[k] = e + v;
	out[k + n/2] = e - v;
    }
}

float Amplitude(float complex c){
    float a = crealf(c);
    float b = cimagf(c);
    return logf(a*a + b*b);
}

float max_amp = 0.0f;

void ProcessAudio(void *buffer, unsigned int frames){
    if (frames < FFT_SIZE) return;
    Frame *frame_data = (Frame*)buffer;

    for (size_t i=0; i<frames; i++){	
	global_raw_in[i] = (frame_data[i].left + frame_data[i].right) / 2.0f;
    }
    FFT(global_raw_in, 1, global_raw_out, FFT_SIZE);
    
    max_amp = 0.0f;
    for (size_t i=0; i<FFT_SIZE; i++){
	float complex c = global_raw_out[i];
	float amp = Amplitude(c);
	if (max_amp < amp ) max_amp = amp;
    }
}

float VISUAL_BUFFER[3][FFT_SIZE] = {0};
void DrawFFT(float x, float y, float w, float h, Color color){
    float dx = w / FFT_SIZE;
    size_t vb_size = sizeof(VISUAL_BUFFER) / sizeof(VISUAL_BUFFER[0]);
    for (size_t i=0; i<FFT_SIZE; i++){
	float complex c = global_raw_out[i];
	float m_amp = Amplitude(c);
	if (max_amp < m_amp) max_amp = m_amp;

	for(size_t j=0;j<vb_size;j++){
	    VISUAL_BUFFER[j][i] = VISUAL_BUFFER[j+1][i];
	}
	VISUAL_BUFFER[vb_size-1][i] = Amplitude(c);
	float f = 0.0f;
	for (size_t j=0; j<vb_size; j++){
	    f += VISUAL_BUFFER[j][i];
	}
	f /= vb_size;

	float amp_norm = clamp(0.0f, 1.0f, f / max_amp);
	float amp_height = amp_norm * h;
	DrawRectangle(x + i*dx, y + h - amp_height, dx, amp_height, color);
    }

}

int main(int argc, char **argv){

    if (argc < 2){
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file){
        printf("file %s does not exist\n", argv[1]);
        return 1;
    }
    fclose(file);
    int screenWidth = 800;
    int screenHeight = 480;
    char* title = "Music Player";
    InitWindow(screenWidth, screenHeight, title);
    SetTargetFPS(60);
    
    pi = 4.0f * atan2f(1.0f, 1.0f);

    InitAudioDevice();
    Music music = LoadMusicStream(argv[1]);
    
    char *music_title = argv[1];
    //  get last
    music_title = strrchr(music_title, '/');
    if (music_title) music_title++;
    else music_title = argv[1];

    AttachAudioStreamProcessor(music.stream, ProcessAudio);
    PlayMusicStream(music);
    char playsound = 1;
    char drawfps = 0;
    while(!WindowShouldClose()){	
	
        if(playsound){
            UpdateMusicStream(music);
        };
        if (IsKeyPressed(KEY_SPACE)){
            playsound = !playsound;
            if (playsound) PlayMusicStream(music);
            else PauseMusicStream(music);
        }
        if (IsKeyPressed(KEY_RIGHT)){
            float seek = GetMusicTimePlayed(music) + 5.0f;
            if (seek > GetMusicTimeLength(music)) seek = GetMusicTimeLength(music);
            SeekMusicStream(music, seek);
        }
        if (IsKeyPressed(KEY_LEFT)){
            float seek = GetMusicTimePlayed(music) - 5.0f;
            if (seek < 0.0f) seek = 0.0f;
            SeekMusicStream(music, seek);
        }
        if ( GetMusicTimePlayed(music) >= floor(GetMusicTimeLength(music))){
            StopMusicStream(music);
            playsound = 0;
        }

	if(IsKeyPressed(KEY_F)) drawfps = !drawfps;

	screenWidth = GetScreenWidth();
	screenHeight = GetScreenHeight();

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText(TextFormat("%s %s",(playsound)? "Playing": "Paused", music_title), 20, 20, 20, LIGHTGRAY);
        char timer[20];
        sprintf(timer, "%02i:%02i/%02i:%02i", (int)GetMusicTimePlayed(music)/60, (int)GetMusicTimePlayed(music)%60, (int)GetMusicTimeLength(music)/60, (int)GetMusicTimeLength(music)%60);
        DrawText(timer, screenWidth - 150, 20, 20, LIGHTGRAY);

        DrawRectangle(20, 40, screenWidth - 40, 20, LIGHTGRAY);
        DrawRectangle(20, 40, (screenWidth - 40) * GetMusicTimePlayed(music) / GetMusicTimeLength(music), 20, MAROON);
        
        DrawRectangle(20, 70, screenWidth - 40, screenHeight - 200, LIGHTGRAY);
        DrawFFT(20, 70, screenWidth - 40, screenHeight - 200, MAROON);

	DrawText("Space: Play/Pause | Left/Right: Back/Forward 5 secs", 20, screenHeight - 40, 20, LIGHTGRAY);

	if (drawfps){
	    DrawFPS(screenWidth - 100, screenHeight - 20);
	}

	EndDrawing();

    }
    UnloadMusicStream(music);
    CloseAudioDevice();
    CloseWindow(); 
    return 0;
}
