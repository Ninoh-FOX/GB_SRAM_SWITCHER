#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>


// Definición de teclas
#define BUTTON_A        SDLK_SPACE
#define BUTTON_B        SDLK_LCTRL
#define BUTTON_MENU     SDLK_ESCAPE
#define BUTTON_UP       SDLK_UP
#define BUTTON_DOWN     SDLK_DOWN

// Definición de dimensiones de ventana
#define WINDOW_WIDTH    640
#define WINDOW_HEIGHT   480

typedef struct {
    char** fileList;
    int numFiles;
} FileListData;

SDL_Surface* renderText(const char* text, TTF_Font* font, SDL_Color color) {
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text, color);
    if (textSurface == NULL) {
        printf("Error al renderizar el texto: %s\n", TTF_GetError());
        exit(1);
    }
    return textSurface;
}

// Función personalizada para obtener el nombre de archivo de una ruta
char* getFilename(const char* path) {
    char* filename = NULL;
    const char* lastSlash = strrchr(path, '/');
    if (lastSlash != NULL) {
        filename = strdup(lastSlash + 1);
    } else {
        filename = strdup(path);
    }
    return filename;
}

// Función auxiliar para obtener el nombre del archivo sin la extensión
char* getFileNameWithoutExtension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    size_t length = (dot != NULL) ? (dot - filename) : strlen(filename);
    char* name = (char*)malloc(length + 1);
    strncpy(name, filename, length);
    name[length] = '\0';
    return name;
}

void listFiles(const char* dirPath, FileListData* fileData) {
    struct dirent** namelist;
    int numFiles = scandir(dirPath, &namelist, NULL, alphasort);
    if (numFiles < 0) {
        printf("No se pudo abrir el directorio: %s\n", dirPath);
        return;
    }

    fileData->numFiles = 0;
    for (int i = 0; i < numFiles; i++) {
        if (namelist[i]->d_name[0] != '.') {
            char* fileName = getFileNameWithoutExtension(namelist[i]->d_name);
            char* dot = strrchr(namelist[i]->d_name, '.');
            if (dot != NULL && strcmp(dot, ".srm") == 0) {
                fileData->fileList[fileData->numFiles] = fileName;
                (fileData->numFiles)++;
            } else {
                free(fileName);
            }
            free(namelist[i]);
        }
    }
    free(namelist);
}

void freeFileList(FileListData* fileData) {
    for (int i = 0; i < fileData->numFiles; i++) {
        free(fileData->fileList[i]);
    }
    free(fileData->fileList);
    fileData->numFiles = 0; // Reiniciar el contador de archivos
}


void switchData(const char* filePath) {
    printf("Ejecutando switchData para el archivo: %s\n", filePath);

    // Leer los datos SRM
    char srmFilePath[PATH_MAX];
    snprintf(srmFilePath, sizeof(srmFilePath), "%s.srm", filePath);
    FILE* srmFile = fopen(srmFilePath, "rb");
    if (srmFile == NULL) {
        printf("No se pudo abrir el archivo SRM: %s\n", srmFilePath);
        return;
    }

    fseek(srmFile, 0, SEEK_END);
    size_t srmFileSize = ftell(srmFile);
    rewind(srmFile);

    unsigned char* srmData = (unsigned char*)malloc(srmFileSize);
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
    char rtcFilePath[PATH_MAX];
    snprintf(rtcFilePath, sizeof(rtcFilePath), "%s.rtc", filePath);
    FILE* rtcFile = fopen(rtcFilePath, "rb");
    if (rtcFile == NULL) {
        printf("No se pudo abrir el archivo RTC: %s\n", rtcFilePath);
        free(srmData);
        return;
    }

    fseek(rtcFile, 0, SEEK_END);
    size_t rtcFileSize = ftell(rtcFile);
    rewind(rtcFile);

    unsigned char* rtcData = (unsigned char*)malloc(rtcFileSize);
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
    unsigned char* tempSram1 = (unsigned char*)malloc(0x2000);
    unsigned char* tempSram2 = (unsigned char*)malloc(0x2000);
    unsigned char* tempRtc1 = (unsigned char*)malloc(0x200);
    unsigned char* tempRtc2 = (unsigned char*)malloc(0x200);

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
    char newSrmFilePath[PATH_MAX];
    snprintf(newSrmFilePath, sizeof(newSrmFilePath), "%s/.netplay/%s.srm", filePath, getFileNameWithoutExtension(filePath));
    printf("Ruta de destino SRM: %s\n", newSrmFilePath);

    FILE* newSrmFile = fopen(newSrmFilePath, "wb");
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

    // Construir la ruta completa del archivo RTC en la carpeta .netplay
    char newRtcFilePath[PATH_MAX];
    snprintf(newRtcFilePath, sizeof(newRtcFilePath), "%s/.netplay/%s.rtc", filePath, getFileNameWithoutExtension(filePath));
    printf("Ruta de destino RTC: %s\n", newRtcFilePath);

    FILE* newRtcFile = fopen(newRtcFilePath, "wb");
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

void restoreData(const char* filePath) {
    printf("Ejecutando restoreData para el archivo: %s\n", filePath);

    // Leer los datos SRM
    char srmFilePath[PATH_MAX];
    snprintf(srmFilePath, sizeof(srmFilePath), "%s.srm", filePath);
    FILE* srmFile = fopen(srmFilePath, "rb");
    if (srmFile == NULL) {
        printf("No se pudo abrir el archivo SRM: %s\n", srmFilePath);
        return;
    }

    fseek(srmFile, 0, SEEK_END);
    size_t srmFileSize = ftell(srmFile);
    rewind(srmFile);

    unsigned char* srmData = (unsigned char*)malloc(srmFileSize);
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
    char rtcFilePath[PATH_MAX];
    snprintf(rtcFilePath, sizeof(rtcFilePath), "%s.rtc", filePath);
    FILE* rtcFile = fopen(rtcFilePath, "rb");
    if (rtcFile == NULL) {
        printf("No se pudo abrir el archivo RTC: %s\n", rtcFilePath);
        free(srmData);
        return;
    }

    fseek(rtcFile, 0, SEEK_END);
    size_t rtcFileSize = ftell(rtcFile);
    rewind(rtcFile);

    unsigned char* rtcData = (unsigned char*)malloc(rtcFileSize);
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
    unsigned char* tempSram1 = (unsigned char*)malloc(0x2000);
    unsigned char* tempSram2 = (unsigned char*)malloc(0x2000);
    unsigned char* tempRtc1 = (unsigned char*)malloc(0x200);
    unsigned char* tempRtc2 = (unsigned char*)malloc(0x200);

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

    // Construir la ruta completa del archivo SRM original
    char originalSrmFilePath[PATH_MAX];
    snprintf(originalSrmFilePath, sizeof(originalSrmFilePath), "%s/.netplay/%s.srm", filePath, getFileNameWithoutExtension(filePath));
    printf("Ruta del archivo SRM original: %s\n", originalSrmFilePath);

    FILE* originalSrmFile = fopen(originalSrmFilePath, "wb");
    if (originalSrmFile == NULL) {
        printf("No se pudo abrir el archivo SRM original para escritura\n");
        free(srmData);
        free(rtcData);
        return;
    }

    if (fwrite(srmData, 1, srmFileSize, originalSrmFile) != srmFileSize) {
        printf("Error al restaurar los datos SRM\n");
        fclose(originalSrmFile);
        free(srmData);
        free(rtcData);
        return;
    }

    fclose(originalSrmFile);

    // Construir la ruta completa del archivo RTC original
    char originalRtcFilePath[PATH_MAX];
    snprintf(originalRtcFilePath, sizeof(originalRtcFilePath), "%s/.netplay/%s.rtc", filePath, getFileNameWithoutExtension(filePath));
    printf("Ruta del archivo RTC original: %s\n", originalRtcFilePath);

    FILE* originalRtcFile = fopen(originalRtcFilePath, "wb");
    if (originalRtcFile == NULL) {
        printf("No se pudo abrir el archivo RTC original para escritura\n");
        free(srmData);
        free(rtcData);
        return;
    }

    if (fwrite(rtcData, 1, rtcFileSize, originalRtcFile) != rtcFileSize) {
        printf("Error al restaurar los datos RTC\n");
        fclose(originalRtcFile);
        free(srmData);
        free(rtcData);
        return;
    }

    fclose(originalRtcFile);

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
    SDL_Surface* screen = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 0, SDL_DOUBLEBUF);
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
    TTF_Font* font = TTF_OpenFont("Exo-2-Bold-Italic.ttf", 24);
    if (font == NULL) {
        printf("No se pudo cargar la fuente: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Crear los colores
    SDL_Color colorWhite = { 255, 255, 255 };
    SDL_Color colorRed = { 255, 0, 0 };

    // Variables para el control del bucle principal
    bool running = true;
    int currentScreen = 0; // 0: Menu principal, 1: Lista de archivos
    int selectedOption = 0;
    FileListData fileListData;
    fileListData.numFiles = 0;
    const int MAX_FILES = 1024;
    fileListData.fileList = (char**)malloc(sizeof(char*) * MAX_FILES); // Asegurar suficiente espacio para MAX_FILES nombres de archivo
    if (fileListData.fileList == NULL) {
        printf("No se pudo asignar memoria para la lista de archivos\n");
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Rutas de directorios
    const char* srmDirPath0 = "/mnt/SDCARD/Saves/RA_saves/TGB Dual";
    const char* srmDirPath1 = "/mnt/SDCARD/Saves/RA_saves/TGB Dual/.netplay";

    // Variable para almacenar la ruta seleccionada
    char selectedDirPath[PATH_MAX] = ""; // Valor predeterminado

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case BUTTON_UP:
                        if (currentScreen == 0) {
                            if (selectedOption > 0) {
                                selectedOption--;
                            }
                        } else if (currentScreen == 1) {
                            if (selectedOption > 0) {
                                selectedOption--;
                            }
                        }
                        break;
                    case BUTTON_DOWN:
                        if (currentScreen == 0) {
                            if (selectedOption < 2) {
                                selectedOption++;
                            }
                        } else if (currentScreen == 1) {
                            if (selectedOption < fileListData.numFiles - 1) {
                                selectedOption++;
                            }
                        }
                        break;
                    case BUTTON_A:
                        if (currentScreen == 0) {
                            if (selectedOption == 0) {
                                currentScreen = 1;
                                selectedOption = 0;
                                fileListData.numFiles = 0;
                                listFiles(srmDirPath0, &fileListData);
                                strcpy(selectedDirPath, srmDirPath0); // Actualizar selectedDirPath
                            } else if (selectedOption == 1) {
                                currentScreen = 1;
                                selectedOption = 0;
                                fileListData.numFiles = 0;
                                listFiles(srmDirPath1, &fileListData);
                                strcpy(selectedDirPath, srmDirPath1); // Actualizar selectedDirPath
                            } else if (selectedOption == 2) {
                                printf("Opción de salida seleccionada\n");
                                running = false;
                            }
                        } else if (currentScreen == 1) {
                            if (selectedOption >= 0 && selectedOption < fileListData.numFiles) {
                                char filePath[256];
                                snprintf(filePath, sizeof(filePath), "%s/%s", selectedDirPath, fileListData.fileList[selectedOption]);
                                
                                if (strcmp(selectedDirPath, srmDirPath0) == 0) {
                                    switchData(filePath);
                                }

                                if (strcmp(selectedDirPath, srmDirPath1) == 0) {
                                    restoreData(filePath);
                                }
                                
                                // Mostrar mensaje de confirmación
                                printf("Datos modificados correctamente\n");
                                currentScreen = 0;
                                selectedOption = 0;
                                fileListData.numFiles = 0;
                                strcpy(selectedDirPath, ""); // Reiniciar selectedDirPath
                            }
                        }
                        break;
                    case BUTTON_B:
                        if (currentScreen == 0) {
                            printf("Botón B seleccionado\n");
                            running = false;
                        }

                        if (currentScreen == 1) {
                            printf("Volver al menú principal\n");
                            currentScreen = 0;
                            selectedOption = 0;
                            fileListData.numFiles = 0;
                            freeFileList(&fileListData); // Liberar la memoria del listado de archivos
                            strcpy(selectedDirPath, ""); // Reiniciar selectedDirPath
                        }
                        break;
                    case BUTTON_MENU:
                        if (currentScreen == 0) {
                            printf("Cerrando el programa\n");
                            running = false;
                        }
                        break;
                    default:
                        break;
                }
            }
        }

        // Limpiar pantalla
        SDL_FillRect(screen, NULL, 0x000000);

        // Mostrar el menú principal
        if (currentScreen == 0) {
            SDL_Surface* titleSurface = renderText("GameBoy SRAM Switcher", font, colorWhite);
            SDL_Rect titleRect;
            titleRect.x = WINDOW_WIDTH / 2 - titleSurface->w / 2;
            titleRect.y = 50;
            SDL_BlitSurface(titleSurface, NULL, screen, &titleRect);
            SDL_FreeSurface(titleSurface);

            SDL_Surface* option1Surface = renderText("Intercambiar datos guardados", font, (selectedOption == 0) ? colorRed : colorWhite);
            SDL_Rect option1Rect;
            option1Rect.x = WINDOW_WIDTH / 2 - option1Surface->w / 2;
            option1Rect.y = 200;
            SDL_BlitSurface(option1Surface, NULL, screen, &option1Rect);
            SDL_FreeSurface(option1Surface);

            SDL_Surface* option2Surface = renderText("Restaurar datos guardados", font, (selectedOption == 1) ? colorRed : colorWhite);
            SDL_Rect option2Rect;
            option2Rect.x = WINDOW_WIDTH / 2 - option2Surface->w / 2;
            option2Rect.y = 250;
            SDL_BlitSurface(option2Surface, NULL, screen, &option2Rect);
            SDL_FreeSurface(option2Surface);

            SDL_Surface* option3Surface = renderText("Salir", font, (selectedOption == 2) ? colorRed : colorWhite);
            SDL_Rect option3Rect;
            option3Rect.x = WINDOW_WIDTH / 2 - option3Surface->w / 2;
            option3Rect.y = 300;
            SDL_BlitSurface(option3Surface, NULL, screen, &option3Rect);
            SDL_FreeSurface(option3Surface);
        }

        // Mostrar la lista de archivos
        if (currentScreen == 1) {
            SDL_Surface* titleSurface = renderText("Seleccione un archivo:", font, colorWhite);
            SDL_Rect titleRect;
            titleRect.x = WINDOW_WIDTH / 2 - titleSurface->w / 2;
            titleRect.y = 50;
            SDL_BlitSurface(titleSurface, NULL, screen, &titleRect);
            SDL_FreeSurface(titleSurface);

            for (int i = 0; i < fileListData.numFiles; i++) {
                SDL_Color textColor = (selectedOption == i) ? colorRed : colorWhite;
                SDL_Surface* fileSurface = renderText(fileListData.fileList[i], font, textColor);
                SDL_Rect fileRect;
                fileRect.x = WINDOW_WIDTH / 2 - fileSurface->w / 2;
                fileRect.y = 150 + i * 50;
                SDL_BlitSurface(fileSurface, NULL, screen, &fileRect);
                SDL_FreeSurface(fileSurface);
            }
        }

        // Actualizar la pantalla
        SDL_Flip(screen);
    }

    // Liberar memoria y cerrar SDL al finalizar
    freeFileList(&fileData);
    TTF_CloseFont(font);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
