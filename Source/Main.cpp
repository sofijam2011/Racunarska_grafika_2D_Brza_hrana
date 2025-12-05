#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include "../Header/Util.h"

// konstante
const int TARGET_FPS = 75;
const double FRAME_TIME = 1.0 / TARGET_FPS;
const float MOVE_SPEED = 0.8f;

// stanja
enum GameState {
    STATE_MENU,
    STATE_COOKING,
    STATE_ASSEMBLY,
    STATE_FINISHED
};

GameState currentState = STATE_MENU;

// sastojci
enum Ingredient {
    ING_DONJA_ZEMICKA,   // 0
    ING_PLJESKAVICA,     // 1
    ING_KECAP,           // 2
    ING_SENF,            // 3
    ING_KRASTAVCICI,     // 4
    ING_LUK,             // 5
    ING_SALATA,          // 6
    ING_SIR,             // 7
    ING_PARADAJZ,        // 8
    ING_GORNJA_ZEMICKA,  // 9
    ING_TOTAL            // 10 (ukupan broj)
};

// globalne promenljive
int wWidth = 800;
int wHeight = 600;

// mis
double mouseX = 0.0;
double mouseY = 0.0;
bool mouseClicked = false;

// tastatura - WASD i Space
bool keyW = false;
bool keyA = false;
bool keyS = false;
bool keyD = false;
bool keySpace = false;
bool keySpaceJustPressed = false;

// cooking faza
float pattyX = 0.0f;
float pattyY = 0.3f;
float cookProgress = 0.0f;

// after cooking faza
int currentIngredient = 0;      // koji sastojak je aktivan
float ingredientX = 0.0f;       // pozicija aktivnog sastojka
float ingredientY = 0.5f;
int ingredientsPlaced = 0;      // koliko je vec postavljeno na tanjir

// tanjir
float plateX = -0.15f;
float plateY = -0.65f;
float plateW = 0.3f;
float plateH = 0.08f;

// barice (promaseni kecap/senf)
struct Puddle {
    float x, y;
    bool isKetchup;  // true = crvena, false = zuta
};
std::vector<Puddle> puddles;

// pomocne funkcije
float mouseToNDC_X() {
    return (2.0f * (float)mouseX / wWidth) - 1.0f;
}

float mouseToNDC_Y() {
    return 1.0f - (2.0f * (float)mouseY / wHeight);
}

bool pointInRect(float px, float py, float rx, float ry, float rw, float rh) {
    return (px >= rx && px <= rx + rw && py >= ry && py <= ry + rh);
}

float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// funkcija za renderovanje zemicke sa detaljima
void drawBun(float x, float y, float w, float h, bool isTop, int uPosLoc, int uScaleLoc, int uColorLoc) {
    // boje zemicke - razlicite nijanse da izgleda realnije
    float bunBase = 0.85f, bunMid = 0.65f, bunDark = 0.55f;  // odnos boja
    
    if (isTop) {
        // gornja zemicka - kupolast vrh (najsiri na dnu gde se lepi na burger)
        
        // sloj 1: donji deo - najsiri (celom sirinom)
        glUniform2f(uPosLoc, x, y);
        glUniform2f(uScaleLoc, w, h * 0.25f);
        glUniform4f(uColorLoc, bunBase, bunMid, bunDark * 0.7f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // sloj 2: srednji deo - malo uzi
        glUniform2f(uPosLoc, x + w * 0.08f, y + h * 0.25f);
        glUniform2f(uScaleLoc, w * 0.84f, h * 0.35f);
        glUniform4f(uColorLoc, bunBase, bunMid, bunDark, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // sloj 3: gornji srednji - jos uzi
        glUniform2f(uPosLoc, x + w * 0.12f, y + h * 0.6f);
        glUniform2f(uScaleLoc, w * 0.76f, h * 0.25f);
        glUniform4f(uColorLoc, bunBase * 0.95f, bunMid * 0.9f, bunDark * 0.8f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // sloj 4: vrh - najuzi, najsvetliji (kora)
        glUniform2f(uPosLoc, x + w * 0.18f, y + h * 0.85f);
        glUniform2f(uScaleLoc, w * 0.64f, h * 0.15f);
        glUniform4f(uColorLoc, bunBase * 0.85f, bunMid * 0.75f, bunDark * 0.6f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // semenke na gornjoj zemickoj (rasporedjene po sredini i vrhu)
        for (int i = 0; i < 9; i++) {
            // semenke na srednjem delu
            if (i < 5) {
                float seedX = x + w * (0.2f + (i % 3) * 0.2f);
                float seedY = y + h * (0.35f + (i / 3) * 0.2f);
                glUniform2f(uPosLoc, seedX, seedY);
                glUniform2f(uScaleLoc, w * 0.025f, h * 0.1f);
                glUniform4f(uColorLoc, 0.95f, 0.92f, 0.88f, 1.0f);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
            // semenke na gornjem delu
            else {
                float seedX = x + w * (0.25f + ((i - 5) % 2) * 0.25f);
                float seedY = y + h * (0.65f + ((i - 5) / 2) * 0.15f);
                glUniform2f(uPosLoc, seedX, seedY);
                glUniform2f(uScaleLoc, w * 0.02f, h * 0.08f);
                glUniform4f(uColorLoc, 0.92f, 0.89f, 0.85f, 1.0f);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        }
        
    } else {
        // donja zemicka - obrnuta piramida (najuza na dnu, najsira na vrhu gde se slazu sastojci)
        
        // sloj 1: donji deo - najuzi (dno)
        glUniform2f(uPosLoc, x + w * 0.15f, y);
        glUniform2f(uScaleLoc, w * 0.7f, h * 0.25f);
        glUniform4f(uColorLoc, bunBase * 0.88f, bunMid * 0.85f, bunDark * 0.75f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // sloj 2: donji srednji - siri
        glUniform2f(uPosLoc, x + w * 0.1f, y + h * 0.25f);
        glUniform2f(uScaleLoc, w * 0.8f, h * 0.3f);
        glUniform4f(uColorLoc, bunBase * 0.92f, bunMid * 0.9f, bunDark * 0.82f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // sloj 3: gornji srednji - jos siri
        glUniform2f(uPosLoc, x + w * 0.05f, y + h * 0.55f);
        glUniform2f(uScaleLoc, w * 0.9f, h * 0.3f);
        glUniform4f(uColorLoc, bunBase, bunMid, bunDark, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // sloj 4: vrh - najsiri (tamo gde se slazu sastojci) - svetlija boja jer je presek
        glUniform2f(uPosLoc, x, y + h * 0.85f);
        glUniform2f(uScaleLoc, w, h * 0.15f);
        glUniform4f(uColorLoc, bunBase * 1.05f, bunMid * 1.05f, bunDark * 1.05f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        // tekstura sara na donjoj zemicki (tackice)
        for (int i = 0; i < 6; i++) {
            float dotX = x + w * (0.15f + (i % 3) * 0.25f);
            float dotY = y + h * (0.35f + (i / 3) * 0.25f);
            glUniform2f(uPosLoc, dotX, dotY);
            glUniform2f(uScaleLoc, w * 0.015f, h * 0.06f);
            glUniform4f(uColorLoc, bunBase * 0.8f, bunMid * 0.75f, bunDark * 0.65f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }
}

// callback funkcije
void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    wWidth = width;
    wHeight = height;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // WASD
    if (key == GLFW_KEY_W) keyW = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_A) keyA = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_S) keyS = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_D) keyD = (action != GLFW_RELEASE);

    // Space
    if (key == GLFW_KEY_SPACE) {
        if (action == GLFW_PRESS) {
            keySpace = true;
            keySpaceJustPressed = true;
        } else if (action == GLFW_RELEASE) {
            keySpace = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        mouseClicked = true;
    }
}

int main() {
    if (!glfwInit()) {
        std::cout << "GLFW GRESKA!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // fullscreen
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    wWidth = mode->width;
    wHeight = mode->height;

    GLFWwindow* window = glfwCreateWindow(wWidth, wHeight, "Brza Hrana", monitor, NULL);
    if (window == NULL) {
        std::cout << "PROZOR GRESKA!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW GRESKA!" << std::endl;
        return -1;
    }

    std::cout << "OpenGL verzija: " << glGetString(GL_VERSION) << std::endl;

    // sejder
    unsigned int shader = createShader("Shaders/basic.vert", "Shaders/basic.frag");
    int uPosLoc = glGetUniformLocation(shader, "uPos");
    int uScaleLoc = glGetUniformLocation(shader, "uScale");
    int uColorLoc = glGetUniformLocation(shader, "uColor");
    int uUseTextureLoc = glGetUniformLocation(shader, "uUseTexture");
    int uAlphaLoc = glGetUniformLocation(shader, "uAlpha");

	// geometrija za objekte
    float vertices[] = {
        0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 0.0f,   1.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 1.0f,
        0.0f, 1.0f,   0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

	// teksture
    unsigned int texStudentInfo = loadImageToTexture("Textures/student_info.png");
    if (texStudentInfo != 0) {
        glBindTexture(GL_TEXTURE_2D, texStudentInfo);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cout << "Student info tekstura ucitana uspesno!" << std::endl;
    } else {
        std::cout << "GRESKA: Student info tekstura nije ucitana!" << std::endl;
    }

    unsigned int texPrijatno = loadImageToTexture("Textures/prijatno.png");
    if (texPrijatno != 0) {
        glBindTexture(GL_TEXTURE_2D, texPrijatno);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cout << "Prijatno tekstura ucitana uspesno!" << std::endl;
    } else {
        std::cout << "Greska: Prijatno tekstura nije ucitana!" << std::endl;
    }

    unsigned int texKetchup = loadImageToTexture("Textures/kecap.png");
    if (texKetchup != 0) {
        glBindTexture(GL_TEXTURE_2D, texKetchup);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cout << "Ketchup tekstura ucitana uspesno!" << std::endl;
    } else {
        std::cout << "Greska: Kecap tekstura nije ucitana!" << std::endl;
    }

    unsigned int texMustard = loadImageToTexture("Textures/senf.png");
    if (texMustard != 0) {
        glBindTexture(GL_TEXTURE_2D, texMustard);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        std::cout << "Mustard tekstura ucitana uspesno!" << std::endl;
    } else {
        std::cout << "Greska: Senf tekstura nije ucitana!" << std::endl;
    }

    // postavke
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// dugme u meniju
    float btnX = -0.3f, btnY = -0.1f, btnW = 0.6f, btnH = 0.2f;

    std::cout << "Menu: Klikni na dugme za pocetak!" << std::endl;

    // timing
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        double dt = deltaTime.count();

        glfwPollEvents();

        // update
        if (currentState == STATE_MENU) {
            if (mouseClicked) {
                float mx = mouseToNDC_X();
                float my = mouseToNDC_Y();
                if (pointInRect(mx, my, btnX, btnY, btnW, btnH)) {
                    std::cout << "Prelazim u cooking fazu!" << std::endl;
                    currentState = STATE_COOKING;
                    pattyX = -0.1f;
                    pattyY = 0.3f;
                    cookProgress = 0.0f;
                }
                mouseClicked = false;
            }
        }
        else if (currentState == STATE_COOKING) {
            // pomeranje pljeskavice - WASD
            if (keyW) pattyY += MOVE_SPEED * (float)dt;
            if (keyS) pattyY -= MOVE_SPEED * (float)dt;
            if (keyA) pattyX -= MOVE_SPEED * (float)dt;
            if (keyD) pattyX += MOVE_SPEED * (float)dt;

            // pozicija i dimenzije ringle
            float ringlaX = -0.25f;
            float ringlaY = -0.72f;
            float ringlaW = 0.5f;
            float ringlaH = 0.08f;

            // dimenzije pljeskavice
            float pattyW = 0.2f;
            float pattyH = 0.1f;

            // granice kretanja
            if (pattyX < -0.85f) pattyX = -0.85f;
            if (pattyX > 0.65f) pattyX = 0.65f;
            if (pattyY > 0.75f) pattyY = 0.75f;

            // donja granica - zavisi da li je iznad ringle ili ne
            float ringlaTop = ringlaY + ringlaH;
            bool iznadRingle = (pattyX + pattyW > ringlaX) && (pattyX < ringlaX + ringlaW);

            if (iznadRingle) {
                if (pattyY < ringlaTop) pattyY = ringlaTop;
            } else {
                if (pattyY < -0.6f) pattyY = -0.6f;
            }

            // da li pljeskavica dodiruje ringlu?
            bool touchingStove = iznadRingle && (pattyY <= ringlaTop + 0.01f);

            // ako dodiruje, peci
            if (touchingStove && cookProgress < 1.0f) {
                cookProgress += 0.12f * (float)dt;
                if (cookProgress >= 1.0f) {
                    cookProgress = 1.0f;
                    std::cout << "Pljeskavica ispecena! Prelazim u ASSEMBLY..." << std::endl;
                    currentState = STATE_ASSEMBLY;
                    currentIngredient = ING_DONJA_ZEMICKA;
                    ingredientX = 0.0f;
                    ingredientY = 0.5f;
                    ingredientsPlaced = 0;
                }
            }
        }
        else if (currentState == STATE_ASSEMBLY) {
            // pomeranje sastojka - WASD
            if (keyW) ingredientY += MOVE_SPEED * (float)dt;
            if (keyS) ingredientY -= MOVE_SPEED * (float)dt;
            if (keyA) ingredientX -= MOVE_SPEED * (float)dt;
            if (keyD) ingredientX += MOVE_SPEED * (float)dt;

            // granice kretanja
            if (ingredientX < -0.9f) ingredientX = -0.9f;
            if (ingredientX > 0.7f) ingredientX = 0.7f;
            if (ingredientY > 0.8f) ingredientY = 0.8f;

            // dimenzije sastojka 
            float ingW = 0.2f;
            float ingH = 0.06f;

            // vrh tanjira + visina vec postavljenih sastojaka 
            float stackTop = plateY + plateH;
            for (int i = 0; i < ingredientsPlaced; i++) {
                float sh = 0.04f;
                switch (i) {
                    case ING_DONJA_ZEMICKA: sh = 0.05f; break;
                    case ING_PLJESKAVICA:   sh = 0.04f; break;
                    case ING_KECAP:         sh = 0.015f; break;
                    case ING_SENF:          sh = 0.015f; break;
                    case ING_KRASTAVCICI:   sh = 0.03f; break;
                    case ING_LUK:           sh = 0.03f; break;
                    case ING_SALATA:        sh = 0.035f; break;
                    case ING_SIR:           sh = 0.025f; break;
                    case ING_PARADAJZ:      sh = 0.03f; break;
                    case ING_GORNJA_ZEMICKA: sh = 0.06f; break;
                }
                stackTop += sh;
            }

            // da li je sastojak iznad tanjira?
            bool iznadTanjira = (ingredientX + ingW > plateX) && (ingredientX < plateX + plateW);

            // donja granica
            if (iznadTanjira) {
                if (ingredientY < stackTop) ingredientY = stackTop;
            } else {
                if (ingredientY < -0.5f) ingredientY = -0.5f;
            }

            // za kecap i senf - pritisak Space
            if (currentIngredient == ING_KECAP || currentIngredient == ING_SENF) {
                // sirina flasice za proveru
                float flasicaW = 0.06f;
                bool flasicaIznadTanjira = (ingredientX + flasicaW > plateX) && (ingredientX < plateX + plateW);

                if (keySpaceJustPressed) {
                    keySpaceJustPressed = false;

                    if (flasicaIznadTanjira) {
                        // uspesno - flasica je horizontalno iznad tanjira
                        std::cout << "Dodato: " << (currentIngredient == ING_KECAP ? "Kecap" : "Senf") << std::endl;
                        ingredientsPlaced++;
                        currentIngredient++;
                        ingredientX = 0.0f;
                        ingredientY = 0.5f;
                    }
                    else {
                        // promaseno - flasica nije iznad tanjira, barica na stolu
                        Puddle p;
                        p.x = ingredientX;
                        p.y = -0.72f;
                        p.isKetchup = (currentIngredient == ING_KECAP);
                        puddles.push_back(p);
                        std::cout << "Promaseno! Barica na stolu." << std::endl;
                    }
                }
            }
            // ostali sastojci - stavljanje na tanjir
            else {
                bool dodirujeTanjir = iznadTanjira && (ingredientY <= stackTop + 0.01f);

                if (dodirujeTanjir) {
                    std::cout << "Postavljeno: sastojak " << currentIngredient << std::endl;
                    ingredientsPlaced++;
                    currentIngredient++;
                    ingredientX = 0.0f;
                    ingredientY = 0.5f;

                    // da li je poslednji sastojak (gornja zemicka)?
                    if (currentIngredient >= ING_TOTAL) {
                        std::cout << "GOTOVO! Prijatno!" << std::endl;
                        currentState = STATE_FINISHED;
                    }
                }
            }

            keySpaceJustPressed = false;
        }
        // STATE_FINISHED - ceka ESC

        // renderovanje
        glUseProgram(shader);
        glBindVertexArray(VAO);

        // STATE_MENU 
        if (currentState == STATE_MENU) {
            glClearColor(0.4f, 0.2f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // okvir dugmeta
            glUniform2f(uPosLoc, btnX - 0.01f, btnY - 0.01f);
            glUniform2f(uScaleLoc, btnW + 0.02f, btnH + 0.02f);
            glUniform4f(uColorLoc, 0.3f, 0.1f, 0.05f, 1.0f);
            glUniform1i(uUseTextureLoc, false);
            glUniform1f(uAlphaLoc, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // dugme
            glUniform2f(uPosLoc, btnX, btnY);
            glUniform2f(uScaleLoc, btnW, btnH);
            glUniform4f(uColorLoc, 0.8f, 0.2f, 0.1f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // tekst simulacija
            glUniform2f(uPosLoc, btnX + 0.1f, btnY + 0.08f);
            glUniform2f(uScaleLoc, btnW - 0.2f, 0.04f);
            glUniform4f(uColorLoc, 1.0f, 0.9f, 0.8f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // STATE_COOKING 
        else if (currentState == STATE_COOKING) {
            glClearColor(0.4f, 0.45f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // sporet
            glUniform2f(uPosLoc, -0.6f, -1.0f);
            glUniform2f(uScaleLoc, 1.2f, 0.35f);
            glUniform4f(uColorLoc, 0.15f, 0.15f, 0.15f, 1.0f);
            glUniform1i(uUseTextureLoc, false);
            glUniform1f(uAlphaLoc, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // ringla
            glUniform2f(uPosLoc, -0.25f, -0.72f);
            glUniform2f(uScaleLoc, 0.5f, 0.08f);
            glUniform4f(uColorLoc, 0.9f, 0.3f, 0.1f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // loading bar - pozadina
            float barX = -0.4f, barY = 0.85f, barW = 0.8f, barH = 0.07f;
            glUniform2f(uPosLoc, barX, barY);
            glUniform2f(uScaleLoc, barW, barH);
            glUniform4f(uColorLoc, 0.3f, 0.3f, 0.3f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // loading bar - progress
            glUniform2f(uPosLoc, barX, barY);
            glUniform2f(uScaleLoc, barW * cookProgress, barH);
            glUniform4f(uColorLoc, 0.2f, 0.85f, 0.2f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // pljeska
            float rawR = 0.85f, rawG = 0.5f, rawB = 0.5f;
            float cookedR = 0.4f, cookedG = 0.22f, cookedB = 0.12f;
            float pR = lerp(rawR, cookedR, cookProgress);
            float pG = lerp(rawG, cookedG, cookProgress);
            float pB = lerp(rawB, cookedB, cookProgress);

            glUniform2f(uPosLoc, pattyX, pattyY);
            glUniform2f(uScaleLoc, 0.2f, 0.1f);
            glUniform4f(uColorLoc, pR, pG, pB, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // STATE_ASSEMBLY 
        else if (currentState == STATE_ASSEMBLY) {
            // pozadina 
            glClearColor(0.5f, 0.45f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // sto 
            glUniform2f(uPosLoc, -1.0f, -1.0f);
            glUniform2f(uScaleLoc, 2.0f, 0.35f);
            glUniform4f(uColorLoc, 0.45f, 0.3f, 0.15f, 1.0f);
            glUniform1i(uUseTextureLoc, false);
            glUniform1f(uAlphaLoc, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // tanjir 
            glUniform2f(uPosLoc, plateX, plateY);
            glUniform2f(uScaleLoc, plateW, plateH);
            glUniform4f(uColorLoc, 0.9f, 0.9f, 0.85f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // barice na stolu (promaseni kecap/senf)
            for (const Puddle& p : puddles) {
                glUniform2f(uPosLoc, p.x, p.y);
                glUniform2f(uScaleLoc, 0.08f, 0.025f);
                if (p.isKetchup) {
                    glUniform4f(uColorLoc, 0.8f, 0.1f, 0.1f, 0.9f);
                } else {
                    glUniform4f(uColorLoc, 0.9f, 0.8f, 0.1f, 0.9f);
                }
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            // vec postavljeni sastojci na tanjiru
            float stackY = plateY + plateH;
            for (int i = 0; i < ingredientsPlaced; i++) {
                int ingType = i;  // redosled = tip sastojka
                float sx = plateX + 0.02f;
                float sw = plateW - 0.04f;
                float sh = 0.04f;

                // boja za svaki sastojak
                float r, g, b;
                switch (ingType) {
                    case ING_DONJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; sh = 0.05f; break;
                    case ING_PLJESKAVICA:   r = 0.4f; g = 0.22f; b = 0.12f; sh = 0.04f; break;
                    case ING_KECAP:         r = 0.8f; g = 0.1f; b = 0.1f; sh = 0.015f; break;  //
                    case ING_SENF:          r = 0.9f; g = 0.8f; b = 0.1f; sh = 0.015f; break;  // 
                    case ING_KRASTAVCICI:   r = 0.5f; g = 0.7f; b = 0.3f; sh = 0.03f; break;
                    case ING_LUK:           r = 0.95f; g = 0.9f; b = 0.95f; sh = 0.03f; break;
                    case ING_SALATA:        r = 0.3f; g = 0.8f; b = 0.3f; sh = 0.035f; break;
                    case ING_SIR:           r = 1.0f; g = 0.75f; b = 0.15f; sh = 0.025f; break; // 
                    case ING_PARADAJZ:      r = 0.95f; g = 0.3f; b = 0.2f; sh = 0.03f; break;  //
                    case ING_GORNJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; sh = 0.06f; break;
                    default: r = 0.5f; g = 0.5f; b = 0.5f; break;
                }

                // za isecene krastavcice
                if (ingType == ING_KRASTAVCICI) {
                    // 5 manjih komada sa razmakom
                    int numPieces = 5;
                    float pieceWidth = (sw / numPieces) * 0.8f;  // 0.8 za razmak
                    float spacing = sw / numPieces;
                    
                    for (int p = 0; p < numPieces; p++) {
                        float pieceX = sx + (p * spacing) + (spacing - pieceWidth) / 2.0f;
                        glUniform2f(uPosLoc, pieceX, stackY);
                        glUniform2f(uScaleLoc, pieceWidth, sh);
                        glUniform4f(uColorLoc, r, g, b, 1.0f);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                } else if (ingType == ING_DONJA_ZEMICKA) {
                    drawBun(sx, stackY, sw, sh, false, uPosLoc, uScaleLoc, uColorLoc);
                } else if (ingType == ING_GORNJA_ZEMICKA) {
                    drawBun(sx, stackY, sw, sh, true, uPosLoc, uScaleLoc, uColorLoc);
                } else {
                    glUniform2f(uPosLoc, sx, stackY);
                    glUniform2f(uScaleLoc, sw, sh);
                    glUniform4f(uColorLoc, r, g, b, 1.0f);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }

                stackY += sh;  // dodajem stvarnu visinu sastojka
            }

            // aktivni sastojak koji se pomera
            if (currentIngredient < ING_TOTAL) {
                float r, g, b;
                float iw = plateW - 0.04f;  
                float ih = 0.05f;

                switch (currentIngredient) {
                    case ING_DONJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; break;
                    case ING_PLJESKAVICA:   r = 0.4f; g = 0.22f; b = 0.12f; break;
                    case ING_KECAP:         r = 0.8f; g = 0.1f; b = 0.1f; iw = 0.06f; ih = 0.15f; break;  // 
                    case ING_SENF:          r = 0.9f; g = 0.8f; b = 0.1f; iw = 0.06f; ih = 0.15f; break;  //
                    case ING_KRASTAVCICI:   r = 0.5f; g = 0.7f; b = 0.3f; break;
                    case ING_LUK:           r = 0.95f; g = 0.9f; b = 0.95f; break;
                    case ING_SALATA:        r = 0.3f; g = 0.8f; b = 0.3f; break;
                    case ING_SIR:           r = 1.0f; g = 0.75f; b = 0.15f; break;  // 
                    case ING_PARADAJZ:      r = 0.95f; g = 0.3f; b = 0.2f; break;   //
                    case ING_GORNJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; ih = 0.07f; break;
                    default: r = 0.5f; g = 0.5f; b = 0.5f; break;
                }

                // za kecap i senf - flašice sa teksturama
                if (currentIngredient == ING_KECAP || currentIngredient == ING_SENF) {
                    glUniform2f(uPosLoc, ingredientX, ingredientY);
                    glUniform2f(uScaleLoc, iw, ih);
                    glUniform1i(uUseTextureLoc, true);
                    glUniform1f(uAlphaLoc, 1.0f);
                    glActiveTexture(GL_TEXTURE0);
                    
                    if (currentIngredient == ING_KECAP && texKetchup != 0) {
                        glBindTexture(GL_TEXTURE_2D, texKetchup);
                    } else if (currentIngredient == ING_SENF && texMustard != 0) {
                        glBindTexture(GL_TEXTURE_2D, texMustard);
                    } else {
                        // fallback - ako tekstura nije ucitana, koristi boju
                        glUniform1i(uUseTextureLoc, false);
                        glUniform4f(uColorLoc, r, g, b, 1.0f);
                    }
                    
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    glUniform1i(uUseTextureLoc, false);
                    glBindTexture(GL_TEXTURE_2D, 0);
                }
                // krastavcici da budu kao delovi
                else if (currentIngredient == ING_KRASTAVCICI) {
                    int numPieces = 5;
                    float pieceWidth = (iw / numPieces) * 0.8f;
                    float spacing = iw / numPieces;
                    
                    for (int p = 0; p < numPieces; p++) {
                        float pieceX = ingredientX + (p * spacing) + (spacing - pieceWidth) / 2.0f;
                        glUniform2f(uPosLoc, pieceX, ingredientY);
                        glUniform2f(uScaleLoc, pieceWidth, ih);
                        glUniform4f(uColorLoc, r, g, b, 1.0f);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                } else if (currentIngredient == ING_DONJA_ZEMICKA) {
                    drawBun(ingredientX, ingredientY, iw, ih, false, uPosLoc, uScaleLoc, uColorLoc);
                } else if (currentIngredient == ING_GORNJA_ZEMICKA) {
                    drawBun(ingredientX, ingredientY, iw, ih, true, uPosLoc, uScaleLoc, uColorLoc);
                } else {
                    glUniform2f(uPosLoc, ingredientX, ingredientY);
                    glUniform2f(uScaleLoc, iw, ih);
                    glUniform4f(uColorLoc, r, g, b, 1.0f);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }
        }

        // STATE_FINISHED 
        else if (currentState == STATE_FINISHED) {
            // pozadina
            glClearColor(0.5f, 0.45f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // sto
            glUniform2f(uPosLoc, -1.0f, -1.0f);
            glUniform2f(uScaleLoc, 2.0f, 0.35f);
            glUniform4f(uColorLoc, 0.45f, 0.3f, 0.15f, 1.0f);
            glUniform1i(uUseTextureLoc, false);
            glUniform1f(uAlphaLoc, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            for (const Puddle& p : puddles) {
                glUniform2f(uPosLoc, p.x, p.y);
                glUniform2f(uScaleLoc, 0.08f, 0.025f);
                if (p.isKetchup) {
                    glUniform4f(uColorLoc, 0.8f, 0.1f, 0.1f, 0.9f);
                } else {
                    glUniform4f(uColorLoc, 0.9f, 0.8f, 0.1f, 0.9f);
                }
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            // tanjir
            glUniform2f(uPosLoc, plateX, plateY);
            glUniform2f(uScaleLoc, plateW, plateH);
            glUniform4f(uColorLoc, 0.9f, 0.9f, 0.85f, 1.0f);
            glUniform1i(uUseTextureLoc, false);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // gotov hamburger 
            float stackY = plateY + plateH;
            for (int i = 0; i < ING_TOTAL; i++) {
                float r, g, b;
                float sh = 0.04f;
                switch (i) {
                    case ING_DONJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; sh = 0.05f; break;
                    case ING_PLJESKAVICA:   r = 0.4f; g = 0.22f; b = 0.12f; sh = 0.04f; break;
                    case ING_KECAP:         r = 0.8f; g = 0.1f; b = 0.1f; sh = 0.015f; break;  //  
                    case ING_SENF:          r = 0.9f; g = 0.8f; b = 0.1f; sh = 0.015f; break;  // 
                    case ING_KRASTAVCICI:   r = 0.5f; g = 0.7f; b = 0.3f; sh = 0.03f; break;
                    case ING_LUK:           r = 0.95f; g = 0.9f; b = 0.95f; sh = 0.03f; break;
                    case ING_SALATA:        r = 0.3f; g = 0.8f; b = 0.3f; sh = 0.035f; break;
                    case ING_SIR:           r = 1.0f; g = 0.75f; b = 0.15f; sh = 0.025f; break; // 
                    case ING_PARADAJZ:      r = 0.95f; g = 0.3f; b = 0.2f; sh = 0.03f; break;  //
                    case ING_GORNJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; sh = 0.06f; break;
                    default: r = 0.5f; g = 0.5f; b = 0.5f; break;
                }
                
                // krastavcici da ostanu kao delovi i na kraju
                if (i == ING_KRASTAVCICI) {
                    int numPieces = 5;
                    float sw = plateW - 0.04f;
                    float pieceWidth = (sw / numPieces) * 0.8f;
                    float spacing = sw / numPieces;
                    
                    glUniform1i(uUseTextureLoc, false);
                    for (int p = 0; p < numPieces; p++) {
                        float pieceX = (plateX + 0.02f) + (p * spacing) + (spacing - pieceWidth) / 2.0f;
                        glUniform2f(uPosLoc, pieceX, stackY);
                        glUniform2f(uScaleLoc, pieceWidth, sh);
                        glUniform4f(uColorLoc, r, g, b, 1.0f);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                } else if (i == ING_DONJA_ZEMICKA) {
                    glUniform1i(uUseTextureLoc, false);
                    drawBun(plateX + 0.02f, stackY, plateW - 0.04f, sh, false, uPosLoc, uScaleLoc, uColorLoc);
                } else if (i == ING_GORNJA_ZEMICKA) {
                    glUniform1i(uUseTextureLoc, false);
                    drawBun(plateX + 0.02f, stackY, plateW - 0.04f, sh, true, uPosLoc, uScaleLoc, uColorLoc);
                } else {
                    glUniform1i(uUseTextureLoc, false);
                    glUniform2f(uPosLoc, plateX + 0.02f, stackY);
                    glUniform2f(uScaleLoc, plateW - 0.04f, sh);
                    glUniform4f(uColorLoc, r, g, b, 1.0f);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
                
                stackY += sh;  // dodajem stvarnu visinu sastojka
            }

            // prijatno!
            if (texPrijatno != 0) {
                // dimenzije teksture u pikselima
                float imgW = 800.0f;   // sirina slike
                float imgH = 200.0f;   // visina slike

                float desiredWidthNDC = 0.8f; 

                float scaleX = desiredWidthNDC;
                float scaleY = desiredWidthNDC * (imgH / imgW) * ((float)wWidth / (float)wHeight);

                // centriranje slike
                float posX = -scaleX / 2.0f;
                float posY = 0.3f;  // iznad burgera

                glUniform2f(uPosLoc, posX, posY);
                glUniform2f(uScaleLoc, scaleX, scaleY);
                glUniform1i(uUseTextureLoc, true);
                glUniform1f(uAlphaLoc, 1.0f);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texPrijatno);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                
                glUniform1i(uUseTextureLoc, false);
                glBindTexture(GL_TEXTURE_2D, 0);
            } else {
				// ako tekstura nije ucitana, prikazi fallback
                std::cout << "Prikazujem fallback za PRIJATNO!" << std::endl;
                
            }
        }

        // studentski podaci - uvek vidljivo
        if (texStudentInfo != 0) {
            // dimenzije teksture u pikselima
            float imgW = 600.0f;  
            float imgH = 300.0f;   

            // koliko prostora zelim da zauzme kartica
            float desiredWidthNDC = 0.48f; //od -1 do 1

            // racunam visinu u NDC tako da se ocuva aspekt slike na ekranu
            // formula izvedena iz mapiranja NDC -> pixeli:
            // (scaleX / scaleY) * (screenWidth / screenHeight) = imgW / imgH
            float scaleX = desiredWidthNDC;
            float scaleY = desiredWidthNDC * (imgH / imgW) * ((float)wWidth / (float)wHeight);

            // da bih karticu smestila u donji desni ugao izracunam x tako da desna ivica bude malo
            //    udaljena od ruba (-1..1), tj. pos.x + scaleX = 1 - marginNDC
            float marginNDC_x = 0.02f; // udaljenost od desnog ruba (u NDC)
            float marginNDC_y = 0.02f; // udaljenost od donjeg ruba (u NDC)

            float posX = 1.0f - scaleX - marginNDC_x;   // bottom-left x tako da desna ivica bude margin od ruba
            float posY = -1.0f + marginNDC_y;           // bottom-left y blizu dna

            // set uniforms i crtanje
            glUniform2f(uPosLoc, posX, posY);
            glUniform2f(uScaleLoc, scaleX, scaleY);
            glUniform1i(uUseTextureLoc, true);
            glUniform1f(uAlphaLoc, 0.7f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texStudentInfo);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        glBindVertexArray(0);
        glUseProgram(0);


        // spatula kursor
        GLFWcursor* compassCursor = loadImageToCursor("./Resources/cursor.png");
        if (compassCursor != nullptr) {
            glfwSetCursor(window, compassCursor);
        }
        else {
            std::cout << "Upozorenje: Kursor spatule nije ucitan. Koristi se default kursor." << std::endl;
        }

        glfwSwapBuffers(window);
        
        // frame limiter
        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = frameEnd - currentTime;
        if (elapsed.count() < FRAME_TIME) {
            std::this_thread::sleep_for(std::chrono::duration<double>(FRAME_TIME - elapsed.count()));
        }
    }

    // cleanup
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shader);
    if (texStudentInfo != 0) glDeleteTextures(1, &texStudentInfo);
    if (texPrijatno != 0) glDeleteTextures(1, &texPrijatno);
    if (texKetchup != 0) glDeleteTextures(1, &texKetchup);
    if (texMustard != 0) glDeleteTextures(1, &texMustard);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}