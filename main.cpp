#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

// Definición de teclas
#define BUTTON_A        SDLK_SPACE
#define BUTTON_B        SDLK_ESCAPE
#define BUTTON_UP       SDLK_UP
#define BUTTON_DOWN     SDLK_DOWN

// Definición de dimensiones de ventana
#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   480

bool fileListGenerated = false;
char **fileList = NULL;
int numFiles = 0;

void listFiles(const char *dirPath, char **fileList, int *numFiles) {
    DIR *dir = opendir(dirPath);
    if (dir == NULL) {
        printf("No se pudo abrir el directorio: %s\n", dirPath);
        return;
    }

    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] != '.') {
            fileList[*numFiles] = strdup(ent->d_name);
            (*numFiles)++;
        }
    }

    closedir(dir);
}

void switchData(const char *filePath) {
    // Leer los datos SRM
    FILE *srmFile = fopen(filePath, "rb");
    if (srmFile == NULL) {
        printf("No se pudo abrir el archivo SRM: %s\n", filePath);
        return;
    }

    fseek(srmFile, 0, SEEK_END);
    size_t srmFileSize = ftell(srmFile);
    rewind(srmFile);

    unsigned char *srmData = (unsigned char *)malloc(srmFileSize);
    if (srmData == NULL) {
        printf("No se pudo asignar memoria para los datos SRM\n");
        fclose(srmFile);
        return;
    }

    if (fread(srmData, 1, srmFileSize, srmFile) != srmFileSize) {
        printf("Error al leer los datos SRM\n");
        fclose(srmFile);
        free(srmData);
        return;
    }

    fclose(srmFile);

    // Leer los datos RTC
    FILE *rtcFile = fopen(filePath, "rb");
    if (rtcFile == NULL) {
        printf("No se pudo abrir el archivo RTC: %s\n", filePath);
        free(srmData);
        return;
    }

    fseek(rtcFile, 0, SEEK_END);
    size_t rtcFileSize = ftell(rtcFile);
    rewind(rtcFile);

    unsigned char *rtcData = (unsigned char *)malloc(rtcFileSize);
    if (rtcData == NULL) {
        printf("No se pudo asignar memoria para los datos RTC\n");
        fclose(rtcFile);
        free(srmData);
        return;
    }

    if (fread(rtcData, 1, rtcFileSize, rtcFile) != rtcFileSize) {
        printf("Error al leer los datos RTC\n");
        fclose(rtcFile);
        free(srmData);
        free(rtcData);
        return;
    }

    fclose(rtcFile);

    // Intercambiar los datos SRM y RTC
    unsigned char *tempSram1 = (unsigned char *)malloc(0x2000);
    unsigned char *tempSram2 = (unsigned char *)malloc(0x2000);
    unsigned char *tempRtc1 = (unsigned char *)malloc(0x200);
    unsigned char *tempRtc2 = (unsigned char *)malloc(0x200);

    // Intercambiar los datos SRM1 y SRM2
    memcpy(tempSram1, srmData, 0x2000);
    memcpy(tempSram2, srmData + 0x2000, 0x2000);
    memcpy(srmData, tempSram2, 0x2000);
    memcpy(srmData + 0x2000, tempSram1, 0x2000);

    // Intercambiar los datos RTC1 y RTC2
    memcpy(tempRtc1, rtcData, 0x200);
    memcpy(tempRtc2, rtcData + 0x200, 0x200);
    memcpy(rtcData, tempRtc2, 0x200);
    memcpy(rtcData + 0x200, tempRtc1, 0x200);

    free(tempSram1);
    free(tempSram2);
    free(tempRtc1);
    free(tempRtc2);

    // Guardar los datos intercambiados en el archivo SRM y RTC
    
    // Construir la ruta completa del archivo SRM en la carpeta .netplay
    char newPath[256];
    snprintf(newPath, sizeof(newPath), "/mnt/SDCARD/Saves/RA_saves/TGB Dual/.netplay/%s", basename(filePath));
    
    FILE *newSrmFile = fopen(newPath, "wb");
    if (newSrmFile == NULL) {
        printf("No se pudo abrir el archivo SRM para escritura\n");
        free(srmData);
        free(rtcData);
        return;
    }

    if (fwrite(srmData, 1, srmFileSize, newSrmFile) != srmFileSize) {
        printf("Error al escribir los datos SRM\n");
        fclose(newSrmFile);
        free(srmData);
        free(rtcData);
        return;
    }

    fclose(newSrmFile);

    FILE *newRtcFile = fopen(newPath, "wb");
    if (newRtcFile == NULL) {
        printf("No se pudo abrir el archivo RTC para escritura\n");
        free(srmData);
        free(rtcData);
        return;
    }

    if (fwrite(rtcData, 1, rtcFileSize, newRtcFile) != rtcFileSize) {
        printf("Error al escribir los datos RTC\n");
        fclose(newRtcFile);
        free(srmData);
        free(rtcData);
        return;
    }

    fclose(newRtcFile);

    free(srmData);
    free(rtcData);

    printf("Los datos SRM y RTC han sido intercambiados exitosamente\n");
}

void restoreData(const char *filePath) {
    // Leer los datos SRM
    FILE *srmFile = fopen(filePath, "rb");
    if (srmFile == NULL) {
        printf("No se pudo abrir el archivo SRM: %s\n", filePath);
        return;
    }

    fseek(srmFile, 0, SEEK_END);
    size_t srmFileSize = ftell(srmFile);
    rewind(srmFile);

    unsigned char *srmData = (unsigned char *)malloc(srmFileSize);
    if (srmData == NULL) {
        printf("No se pudo asignar memoria para los datos SRM\n");
        fclose(srmFile);
        return;
    }

    if (fread(srmData, 1, srmFileSize, srmFile) != srmFileSize) {
        printf("Error al leer los datos SRM\n");
        fclose(srmFile);
        free(srmData);
        return;
    }

    fclose(srmFile);

    // Leer los datos RTC
    FILE *rtcFile = fopen(filePath, "rb");
    if (rtcFile == NULL) {
        printf("No se pudo abrir el archivo RTC: %s\n", filePath);
        free(srmData);
        return;
    }

    fseek(rtcFile, 0, SEEK_END);
    size_t rtcFileSize = ftell(rtcFile);
    rewind(rtcFile);

    unsigned char *rtcData = (unsigned char *)malloc(rtcFileSize);
    if (rtcData == NULL) {
        printf("No se pudo asignar memoria para los datos RTC\n");
        fclose(rtcFile);
        free(srmData);
        return;
    }

    if (fread(rtcData, 1, rtcFileSize, rtcFile) != rtcFileSize) {
        printf("Error al leer los datos RTC\n");
        fclose(rtcFile);
        free(srmData);
        free(rtcData);
        return;
    }

    fclose(rtcFile);

    // Restaurar los datos SRM y RTC
    unsigned char *tempSram1 = (unsigned char *)malloc(0x2000);
    unsigned char *tempSram2 = (unsigned char *)malloc(0x2000);
    unsigned char *tempRtc1 = (unsigned char *)malloc(0x200);
    unsigned char *tempRtc2 = (unsigned char *)malloc(0x200);

    // Intercambiar los datos SRM1 y SRM2
    memcpy(tempSram1, srmData, 0x2000);
    memcpy(tempSram2, srmData + 0x2000, 0x2000);
    memcpy(srmData, tempSram2, 0x2000);
    memcpy(srmData + 0x2000, tempSram1, 0x2000);

    // Intercambiar los datos RTC1 y RTC2
    memcpy(tempRtc1, rtcData, 0x200);
    memcpy(tempRtc2, rtcData + 0x200, 0x200);
    memcpy(rtcData, tempRtc2, 0x200);
    memcpy(rtcData + 0x200, tempRtc1, 0x200);

    free(tempSram1);
    free(tempSram2);
    free(tempRtc1);
    free(tempRtc2);

    // Guardar los datos restaurados en el archivo SRM y RTC
    
    // Construir la ruta completa del archivo SRM en fuera la carpeta .netplay
    char newPath[256];
    snprintf(newPath, sizeof(newPath), "/mnt/SDCARD/Saves/RA_saves/TGB Dual/%s", basename(filePath));
    
    FILE *newSrmFile = fopen(newPath, "wb");
    if (newSrmFile == NULL) {
        printf("No se pudo abrir el archivo SRM para escritura\n");
        free(srmData);
        free(rtcData);
        return;
    }

    if (fwrite(srmData, 1, srmFileSize, newSrmFile) != srmFileSize) {
        printf("Error al escribir los datos SRM\n");
        fclose(newSrmFile);
        free(srmData);
        free(rtcData);
        return;
    }

    fclose(newSrmFile);

    FILE *newRtcFile = fopen(newPath, "wb");
    if (newRtcFile == NULL) {
        printf("No se pudo abrir el archivo RTC para escritura\n");
        free(srmData);
        free(rtcData);
        return;
    }

    if (fwrite(rtcData, 1, rtcFileSize, newRtcFile) != rtcFileSize) {
        printf("Error al escribir los datos RTC\n");
        fclose(newRtcFile);
        free(srmData);
        free(rtcData);
        return;
    }

    fclose(newRtcFile);

    free(srmData);
    free(rtcData);

    printf("Los datos SRM y RTC han sido restaurados exitosamente\n");
}

int main() {
    // Inicializar SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("No se pudo inicializar SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Crear la ventana
    SDL_Surface *screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 0, SDL_DOUBLEBUF);
    if (screen == NULL) {
        printf("No se pudo crear la ventana: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Inicializar SDL_ttf
    if (TTF_Init() < 0) {
        printf("No se pudo inicializar SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // Cargar la fuente
    TTF_Font *font = TTF_OpenFont("Exo-2-Bold-Italic.ttf", 24);
    if (font == NULL) {
        printf("No se pudo cargar la fuente: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Crear los colores
    SDL_Color colorWhite = {255, 255, 255};
    SDL_Color colorRed = {255, 0, 0};

    // Variables para el control del bucle principal
    bool running = true;
    int selectedOption = 0;
    fileList = (char **)malloc(sizeof(char *) * 256);

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case BUTTON_UP:
                        if (selectedOption > 0) {
                            selectedOption--;
                        }
                        break;
                    case BUTTON_DOWN:
                        if (selectedOption < 2) {
                            selectedOption++;
                        }
                        break;
                    case BUTTON_A:
                        switch (selectedOption) {
                            case 0:
                                printf("Seleccionaste 'Intercambiar datos SRM'\n");
                                if (!fileListGenerated) {
                                    listFiles("/mnt/SDCARD/Saves/RA_saves/TGB Dual", fileList, &numFiles);
                                    fileListGenerated = true;
                                }
                                
                                if (numFiles > 0 && numFiles > selectedOption) {
                                    char filePath[256];
                                    snprintf(filePath, sizeof(filePath), "/mnt/SDCARD/Saves/RA_saves/TGB Dual/%s", fileList[selectedOption]);
                                    switchData(filePath);
                                }
                                break;
                            case 1:
                                printf("Seleccionaste 'Restaurar datos SRM'\n");
                                if (!fileListGenerated) {
                                    listFiles("/mnt/SDCARD/Saves/RA_saves/TGB Dual/.netplay", fileList, &numFiles);
                                    fileListGenerated = true;
                                }

                                if (numFiles > 0 && numFiles > selectedOption) {
                                    char filePath[256];
                                    snprintf(filePath, sizeof(filePath), "/mnt/SDCARD/Saves/RA_saves/TGB Dual/.netplay/%s", fileList[selectedOption]);
                                    restoreData(filePath);
                                }
                                break;
                            case 2:
                                running = false;
                                break;
                        }
                        break;
                    case BUTTON_B:
                        printf("Volviendo al menú principal\n");
						fileListGenerated = false;
                        numFiles = 0;
                        selectedOption = 0;
                        break;
                    default:
                        break;
                }
            }
        }

        // Limpiar la pantalla
        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

        // Renderizar el menú principal
        SDL_Surface *textSurface;
        SDL_Rect textRect;

        textSurface = TTF_RenderText_Blended(font, "GameBoy SRAM switcher", colorWhite);
        textRect.x = WINDOW_WIDTH / 2 - textSurface->w / 2;
        textRect.y = 50;
        SDL_BlitSurface(textSurface, NULL, screen, &textRect);
        SDL_FreeSurface(textSurface);

        textSurface = TTF_RenderText_Blended(font, "Intercambiar datos SRM", selectedOption == 0 ? colorRed : colorWhite);
        textRect.x = WINDOW_WIDTH / 2 - textSurface->w / 2;
        textRect.y = 150;
        SDL_BlitSurface(textSurface, NULL, screen, &textRect);
        SDL_FreeSurface(textSurface);

        textSurface = TTF_RenderText_Blended(font, "Restaurar datos SRM", selectedOption == 1 ? colorRed : colorWhite);
        textRect.x = WINDOW_WIDTH / 2 - textSurface->w / 2;
        textRect.y = 200;
        SDL_BlitSurface(textSurface, NULL, screen, &textRect);
        SDL_FreeSurface(textSurface);

        textSurface = TTF_RenderText_Blended(font, "Salir", selectedOption == 2 ? colorRed : colorWhite);
        textRect.x = WINDOW_WIDTH / 2 - textSurface->w / 2;
        textRect.y = 250;
        SDL_BlitSurface(textSurface, NULL, screen, &textRect);
        SDL_FreeSurface(textSurface);

        // Actualizar la ventana
        SDL_Flip(screen);
    }

    // Liberar recursos
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    for (int i = 0; i < numFiles; i++) {
    free(fileList[i]);
    }
    free(fileList);

    return 0;
}
