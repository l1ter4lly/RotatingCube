#include <iostream>
#include "UserInputHandler.hpp"
//#include<Windows.h>
#include<cmath>
#include<vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include<cstring>


//using namespace std;

//     InputHandler Handler;

//     if(Handler.isValid())
//     {
//             std::cout << "Alles Supi!" << std::endl;
//     }
//     else
//     {
//             std::cout << "Input NOT successful, Program aborted" << std::endl;
//     }
int screenWidth = 100;
int screenHeight = 50;
float K2 = 150 ;        // Kameradistanz (3D-Offset)
float K1 = 50;          // Projektionsskalierung
float cubeWidth = 30;   // Würfelgröße in "3D-Einheiten"
float aspectRatio = float(screenHeight) / screenWidth;  // Bildschirmseitenverhältnis

float resolution = 1.0; // Dichte der berechneten Punkte
bool LUMINOSITY = true;

float rotatedX, rotatedY, rotatedZ, ooz;

int xP, yP;
std::vector<char>b(screenWidth * screenHeight,' ');// ASCII-Ausgabepuffer
std::vector<int>c(screenWidth * screenHeight, 33);
std::vector<float>zBuffer(screenWidth*screenHeight, std::numeric_limits<float>::max()); // Tiefenpuffer (Z-Werte)

float A = 1.0;
float B = 1.0;
float C = 0.0;

void plotPlane (float x, float y, float z, char ch)
{
        /*
                 Berechnet Position und Helligkeit eines 3D-Punkts
                        - Transformiert Koordinaten mit einfacher Rotationsmatrix
                        - Projiziert auf 2D-Bildschirm
                        - Berechnet Beleuchtung basierend auf Oberflächennormale
        */
        // 3D-Rotation
        rotatedX = y * (sin(A)*sin(B)*cos(C) + cos(A)*sin(C)) 
                + z * (sin(A)*sin(C) - cos(A)*sin(B)*cos(C)) 
                + x * cos(B)*cos(C);

        rotatedY = y * (cos(A)*cos(C) - sin(A)*sin(B)*sin(C)) 
                + z * (cos(A)*sin(B)*sin(C) + sin(A)*cos(C)) 
                - x * cos(B)*sin(C);

        rotatedZ = -y*sin(A)*cos(B) // Z-Rotation mit Kameradistanz
                + z*cos(A)*cos(B) 
                + x*sin(B) 
                + K2;           //K2 -> Abstand der "Kamera" zum Objekt

        //je kleiner rotatedZ desto näher -> kleinstmöglicher Z-Wert pro "Pixel"
        rotatedZ = std::max(rotatedZ, 0.0001f); // Vermeidet Division durch Null

        ooz = 1 /rotatedZ; // "One over Z" für perspektivische Verkürzung

        // 2D-Bildschirmkoordinaten berechnen
        //screenWidth/2 -> Startwert in der Mitte des Fensters
        //X * K1 geteilt durch z, höhere Z Werte -> größere Entfernung zur "Kamera"
        int xP = std::clamp(int(screenWidth / 2 + K1 * ooz * rotatedX), 0, screenWidth - 1);// Horizontalposition
        int yP = std::clamp(int(screenHeight / 2 + K1 * ooz * rotatedY), 0, screenHeight - 1);// Vertikalposition


        int position =xP+yP*screenWidth;

        float tmpColor = 33;

        if(LUMINOSITY)
        {
                // Oberflächennormale initialisieren (abhängig von der Fläche)
                x = 0;
                y = 0;
                z = 0;

                if(ch == '@')//Vorderseite (+Z)
                {
                        z = -1;  //In Rotation 
                }
                else if(ch == '#')// Rückseite (-Z)
                { 
                        z = +1;
                }
                else if(ch == '$')// Unterseite (-Y)
                {
                        y = - 1 ;
                }
                else if(ch == 'o')// Oberseite (+Y)
                {
                        y = 1;
                }
                else if(ch == '*')// Linke Seite (-X)
                {
                        x = -1;
                }
                else if(ch==' ')// Rechte Seite (+X)
                {
                        x = 1;
                }

                // Rotierte Normale berechnen
                float lumX =  y*(sin(A)*sin(B)*cos(C) + cos(A)*sin(C)) 
                        + z*(sin(A)*sin(C) - cos(A)*sin(B)*cos(C)) 
                        + x*cos(B)*cos(C);

                float lumY = x*(sin(A)*sin(B)*cos(C)-cos(A)*sin(C))
                        + y *(sin(A)*sin(B)*sin(C)+cos(A)*cos(C))
                        + z * (sin(A)*cos(B));

                float lumZ= x*(cos(A)*sin(B)*cos(C)+sin(A)*sin(C))
                        + y *(cos(A)*sin(B)*sin(C)-sin(A)*cos(C))
                        + z * (cos(A)*cos(B));

                //Beleuchtung aus (X) - Y - Z Richtung
                float lum = (-lumY - lumZ);

                if (lum < 0) 
                {
                        lum = 0;
                }

                // ASCII-Helligkeitsmapping
                const char* luminanceChars = ".,-~:;=!*#%@"; // Dunkel → Hell (12 Zeichen)
                        
                int luminance_index = std::clamp(static_cast<int>(lum * (strlen(luminanceChars) - 1)), 0, 11);

                if(luminance_index < 0)
                {
                        luminance_index = 0;
                }
                // Tiefenpuffer-Test
                if(position>= 0 && position < screenWidth *screenHeight)
                {
                        if(rotatedZ < zBuffer[position] || zBuffer[position] == 0)
                        {
                                zBuffer[position] = rotatedZ;
                                b[position] = luminanceChars[luminance_index];
                                c[position] = tmpColor;
                        }
                }
        }
}

int main()
{
        
        while(true)
        {
                // Konsolen-Reset
                std::cout << "\x1b[2J\x1b[H"; // ANSI: Bildschirm löschen & Cursor Home;

                std::fill(b.begin(),b.end(),' ');
                std::fill(zBuffer.begin(), zBuffer.end(), std::numeric_limits<float>::max());
                std::fill(c.begin(),c.end(),33);

                // 1. Front/Back Faces (Z-Ebene)
                for (float i = -cubeWidth; i < cubeWidth; i += resolution) {
                        for (float j = -cubeWidth; j < cubeWidth; j += resolution) {
                                plotPlane(i, j, -cubeWidth, '@');
                                plotPlane(i, j, cubeWidth, '#');
                        }
                }

                // 2. Top/Bottom Faces (Y-Ebene)
                for (float i = -cubeWidth; i < cubeWidth; i += resolution) {
                        for (float j = -cubeWidth; j < cubeWidth; j += resolution) {
                                plotPlane(i, cubeWidth, j, '$');
                                plotPlane(i, -cubeWidth, j, 'o');
                        }
                }

                // 3. Left/Right Faces (X-Ebene)
                for (float i = -cubeWidth; i < cubeWidth; i += resolution) {
                        for (float j = -cubeWidth; j < cubeWidth; j += resolution) {
                                plotPlane(cubeWidth, j, i, '*');
                                plotPlane(-cubeWidth, j, i, '&');
                        }
                }
                // Ausgabepuffer erstellen
                std::string output;
                output.reserve(screenWidth * screenHeight + screenHeight);

                for (int k = 0; k < (screenWidth * screenHeight); ++k)
                {
                        if (k > 0 && k % screenWidth == 0) output += '\n';// Zeilenumbruch nach jeder Zeile
                        output += b[k];
                        //std::cout.put(k % screenWidth ? b[k]: '\n');
                }
                // Frame anzeigen
                std::cout << output;

                // Rotation aktualisieren
                A += 0.02;      // X-Rotationsgeschwindigkeit
                B += 0.03;      // Y-Rotationsgeschwindigkeit
                C += 0.04;      // Z-Rotationsgeschwindigkeit
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
        } 
        return 0;
}
