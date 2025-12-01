#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include "../Header/Util.h"

// ============================================================================
// KONSTANTE
// ============================================================================
const int TARGET_FPS = 75;
const double FRAME_TIME = 1.0 / TARGET_FPS;
const float MOVE_SPEED = 0.8f;

// ============================================================================
// STANJA IGRE
// ============================================================================
enum GameState {
    STATE_MENU,
    STATE_COOKING,
    STATE_ASSEMBLY,
    STATE_FINISHED
};

GameState currentState = STATE_MENU;

// ============================================================================
// SASTOJCI
// ============================================================================
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

// ============================================================================
// GLOBALNE PROMENLJIVE
// ============================================================================
int wWidth = 800;
int wHeight = 600;

// Mis
double mouseX = 0.0;
double mouseY = 0.0;
bool mouseClicked = false;

// Tastatura - WASD + Space
bool keyW = false;
bool keyA = false;
bool keyS = false;
bool keyD = false;
bool keySpace = false;
bool keySpaceJustPressed = false;

// COOKING faza
float pattyX = 0.0f;
float pattyY = 0.3f;
float cookProgress = 0.0f;

// ASSEMBLY faza
int currentIngredient = 0;      // Koji sastojak je aktivan
float ingredientX = 0.0f;       // Pozicija aktivnog sastojka
float ingredientY = 0.5f;
int ingredientsPlaced = 0;      // Koliko je vec postavljeno na tanjir

// Tanjir
float plateX = -0.15f;
float plateY = -0.65f;
float plateW = 0.3f;
float plateH = 0.08f;

// Barice (promaseni kecap/senf)
struct Puddle {
    float x, y;
    bool isKetchup;  // true = crvena, false = zuta
};
std::vector<Puddle> puddles;

// ============================================================================
// POMOCNE FUNKCIJE
// ============================================================================
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

// ============================================================================
// CALLBACK FUNKCIJE
// ============================================================================
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

// ============================================================================
// MAIN
// ============================================================================
int main() {
    if (!glfwInit()) {
        std::cout << "GLFW GRESKA!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // FULLSCREEN
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN); //dodala sam za kursor

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    if (glewInit() != GLEW_OK) {
        std::cout << "GLEW GRESKA!" << std::endl;
        return -1;
    }

    std::cout << "OpenGL verzija: " << glGetString(GL_VERSION) << std::endl;

    // SHADER
    unsigned int shader = createShader("Shaders/basic.vert", "Shaders/basic.frag");
    int uPosLoc = glGetUniformLocation(shader, "uPos");
    int uScaleLoc = glGetUniformLocation(shader, "uScale");
    int uColorLoc = glGetUniformLocation(shader, "uColor");
    int uUseTextureLoc = glGetUniformLocation(shader, "uUseTexture");
    int uAlphaLoc = glGetUniformLocation(shader, "uAlpha");

    // GEOMETRIJA
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

    // TEKSTURE
    unsigned int texStudentInfo = loadImageToTexture("Textures/student_info.png");
    unsigned int texPrijatnoInfo = loadImageToTexture("Textures/prijatno.jpg");
    if (texStudentInfo != 0) {
        glBindTexture(GL_TEXTURE_2D, texStudentInfo);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (texPrijatnoInfo != 0) {
        glBindTexture(GL_TEXTURE_2D, texPrijatnoInfo);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // POSTAVKE
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Dugme
    float btnX = -0.3f, btnY = -0.1f, btnW = 0.6f, btnH = 0.2f;

    std::cout << "MENU: Klikni na dugme za pocetak!" << std::endl;

    // TIMING
    auto lastTime = std::chrono::high_resolution_clock::now();

    // GLAVNA PETLJA
    while (!glfwWindowShouldClose(window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        double dt = deltaTime.count();

        glfwPollEvents();

        // ====================================================================
        // UPDATE
        // ====================================================================
        if (currentState == STATE_MENU) {
            if (mouseClicked) {
                float mx = mouseToNDC_X();
                float my = mouseToNDC_Y();
                if (pointInRect(mx, my, btnX, btnY, btnW, btnH)) {
                    std::cout << "Prelazim u COOKING!" << std::endl;
                    currentState = STATE_COOKING;
                    pattyX = -0.1f;
                    pattyY = 0.3f;
                    cookProgress = 0.0f;
                }
                mouseClicked = false;
            }
        }
        else if (currentState == STATE_COOKING) {
            // Pomeranje pljeskavice - WASD
            if (keyW) pattyY += MOVE_SPEED * (float)dt;
            if (keyS) pattyY -= MOVE_SPEED * (float)dt;
            if (keyA) pattyX -= MOVE_SPEED * (float)dt;
            if (keyD) pattyX += MOVE_SPEED * (float)dt;

            // Pozicija i dimenzije ringle
            float ringlaX = -0.25f;
            float ringlaY = -0.72f;
            float ringlaW = 0.5f;
            float ringlaH = 0.08f;

            // Dimenzije pljeskavice
            float pattyW = 0.2f;
            float pattyH = 0.1f;

            // Granice kretanja
            if (pattyX < -0.85f) pattyX = -0.85f;
            if (pattyX > 0.65f) pattyX = 0.65f;
            if (pattyY > 0.75f) pattyY = 0.75f;

            // Donja granica - zavisi da li je iznad ringle ili ne
            float ringlaTop = ringlaY + ringlaH;
            bool iznadRingle = (pattyX + pattyW > ringlaX) && (pattyX < ringlaX + ringlaW);

            if (iznadRingle) {
                if (pattyY < ringlaTop) pattyY = ringlaTop;
            } else {
                if (pattyY < -0.6f) pattyY = -0.6f;
            }

            // Da li pljeskavica dodiruje ringlu?
            bool touchingStove = iznadRingle && (pattyY <= ringlaTop + 0.01f);

            // Ako dodiruje, peci
            if (touchingStove && cookProgress < 1.0f) {
                cookProgress += 0.12f * (float)dt;
                if (cookProgress >= 1.0f) {
                    cookProgress = 1.0f;
                    std::cout << "Pljeskavica ispečena! Prelazim u ASSEMBLY..." << std::endl;
                    currentState = STATE_ASSEMBLY;
                    currentIngredient = ING_DONJA_ZEMICKA;
                    ingredientX = 0.0f;
                    ingredientY = 0.5f;
                    ingredientsPlaced = 0;
                }
            }
        }
        else if (currentState == STATE_ASSEMBLY) {
            // Pomeranje sastojka - WASD
            if (keyW) ingredientY += MOVE_SPEED * (float)dt;
            if (keyS) ingredientY -= MOVE_SPEED * (float)dt;
            if (keyA) ingredientX -= MOVE_SPEED * (float)dt;
            if (keyD) ingredientX += MOVE_SPEED * (float)dt;

            // Granice kretanja
            if (ingredientX < -0.9f) ingredientX = -0.9f;
            if (ingredientX > 0.7f) ingredientX = 0.7f;
            if (ingredientY > 0.8f) ingredientY = 0.8f;

            // Dimenzije sastojka (varira, ali za proveru koristimo prosek)
            float ingW = 0.2f;
            float ingH = 0.06f;

            // Vrh tanjira + visina vec postavljenih sastojaka
            float stackTop = plateY + plateH + (ingredientsPlaced * 0.045f);

            // Da li je sastojak iznad tanjira?
            bool iznadTanjira = (ingredientX + ingW > plateX) && (ingredientX < plateX + plateW);

            // Donja granica
            if (iznadTanjira) {
                if (ingredientY < stackTop) ingredientY = stackTop;
            } else {
                if (ingredientY < -0.5f) ingredientY = -0.5f;
            }

            // Za kecap i senf - pritisak Space
            if (currentIngredient == ING_KECAP || currentIngredient == ING_SENF) {
                // Sirina flasice za proveru
                float flasicaW = 0.06f;
                bool flasicaIznadTanjira = (ingredientX + flasicaW > plateX) && (ingredientX < plateX + plateW);

                if (keySpaceJustPressed) {
                    keySpaceJustPressed = false;

                    if (flasicaIznadTanjira) {
                        // Uspesno - flasica je horizontalno iznad tanjira
                        std::cout << "Dodato: " << (currentIngredient == ING_KECAP ? "Kecap" : "Senf") << std::endl;
                        ingredientsPlaced++;
                        currentIngredient++;
                        ingredientX = 0.0f;
                        ingredientY = 0.5f;
                    }
                    else {
                        // Promaseno - flasica NIJE iznad tanjira, barica na stolu
                        Puddle p;
                        p.x = ingredientX;
                        p.y = -0.72f;
                        p.isKetchup = (currentIngredient == ING_KECAP);
                        puddles.push_back(p);
                        std::cout << "Promaseno! Barica na stolu." << std::endl;
                    }
                }
            }
            // Ostali sastojci - stavljanje na tanjir
            else {
                bool dodirujeTanjir = iznadTanjira && (ingredientY <= stackTop + 0.01f);

                if (dodirujeTanjir) {
                    std::cout << "Postavljeno: sastojak " << currentIngredient << std::endl;
                    ingredientsPlaced++;
                    currentIngredient++;
                    ingredientX = 0.0f;
                    ingredientY = 0.5f;

                    // Da li je poslednji sastojak (gornja zemicka)?
                    if (currentIngredient >= ING_TOTAL) {
                        std::cout << "GOTOVO! Prijatno!" << std::endl;
                        currentState = STATE_FINISHED;
                    }
                }
            }

            // Reset space flag
            keySpaceJustPressed = false;
        }
        // STATE_FINISHED - ceka ESC

        // ====================================================================
        // RENDEROVANJE
        // ====================================================================
        glUseProgram(shader);
        glBindVertexArray(VAO);

        // --- STATE_MENU ---
        if (currentState == STATE_MENU) {
            glClearColor(0.4f, 0.2f, 0.15f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Okvir dugmeta
            glUniform2f(uPosLoc, btnX - 0.01f, btnY - 0.01f);
            glUniform2f(uScaleLoc, btnW + 0.02f, btnH + 0.02f);
            glUniform4f(uColorLoc, 0.3f, 0.1f, 0.05f, 1.0f);
            glUniform1i(uUseTextureLoc, false);
            glUniform1f(uAlphaLoc, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Dugme
            glUniform2f(uPosLoc, btnX, btnY);
            glUniform2f(uScaleLoc, btnW, btnH);
            glUniform4f(uColorLoc, 0.8f, 0.2f, 0.1f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Tekst simulacija
            glUniform2f(uPosLoc, btnX + 0.1f, btnY + 0.08f);
            glUniform2f(uScaleLoc, btnW - 0.2f, 0.04f);
            glUniform4f(uColorLoc, 1.0f, 0.9f, 0.8f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // --- STATE_COOKING ---
        else if (currentState == STATE_COOKING) {
            glClearColor(0.4f, 0.45f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // SPORET
            glUniform2f(uPosLoc, -0.6f, -1.0f);
            glUniform2f(uScaleLoc, 1.2f, 0.35f);
            glUniform4f(uColorLoc, 0.15f, 0.15f, 0.15f, 1.0f);
            glUniform1i(uUseTextureLoc, false);
            glUniform1f(uAlphaLoc, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // RINGLA
            glUniform2f(uPosLoc, -0.25f, -0.72f);
            glUniform2f(uScaleLoc, 0.5f, 0.08f);
            glUniform4f(uColorLoc, 0.9f, 0.3f, 0.1f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // LOADING BAR - pozadina
            float barX = -0.4f, barY = 0.85f, barW = 0.8f, barH = 0.07f;
            glUniform2f(uPosLoc, barX, barY);
            glUniform2f(uScaleLoc, barW, barH);
            glUniform4f(uColorLoc, 0.3f, 0.3f, 0.3f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // LOADING BAR - progress
            glUniform2f(uPosLoc, barX, barY);
            glUniform2f(uScaleLoc, barW * cookProgress, barH);
            glUniform4f(uColorLoc, 0.2f, 0.85f, 0.2f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // PLJESKAVICA
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

        // --- STATE_ASSEMBLY ---
        else if (currentState == STATE_ASSEMBLY) {
            // Pozadina - restoran
            glClearColor(0.5f, 0.45f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // STO - braon pravougaonik na dnu
            glUniform2f(uPosLoc, -1.0f, -1.0f);
            glUniform2f(uScaleLoc, 2.0f, 0.35f);
            glUniform4f(uColorLoc, 0.45f, 0.3f, 0.15f, 1.0f);
            glUniform1i(uUseTextureLoc, false);
            glUniform1f(uAlphaLoc, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // TANJIR - svetlo sivi/beli
            glUniform2f(uPosLoc, plateX, plateY);
            glUniform2f(uScaleLoc, plateW, plateH);
            glUniform4f(uColorLoc, 0.9f, 0.9f, 0.85f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // BARICE na stolu (promaseni kecap/senf)
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

            // VEC POSTAVLJENI SASTOJCI na tanjiru
            float stackY = plateY + plateH;
            for (int i = 0; i < ingredientsPlaced; i++) {
                int ingType = i;  // Redosled = tip sastojka
                float sx = plateX + 0.02f;
                float sw = plateW - 0.04f;
                float sh = 0.04f;

                // Boja za svaki sastojak
                float r, g, b;
                switch (ingType) {
                    case ING_DONJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; break;
                    case ING_PLJESKAVICA:   r = 0.4f; g = 0.22f; b = 0.12f; break;
                    case ING_KECAP:         r = 0.8f; g = 0.1f; b = 0.1f; sh = 0.02f; break;
                    case ING_SENF:          r = 0.9f; g = 0.8f; b = 0.1f; sh = 0.02f; break;
                    case ING_KRASTAVCICI:   r = 0.5f; g = 0.7f; b = 0.3f; break;
                    case ING_LUK:           r = 0.95f; g = 0.9f; b = 0.95f; break;
                    case ING_SALATA:        r = 0.3f; g = 0.8f; b = 0.3f; break;
                    case ING_SIR:           r = 1.0f; g = 0.85f; b = 0.2f; break;
                    case ING_PARADAJZ:      r = 0.9f; g = 0.2f; b = 0.15f; break;
                    case ING_GORNJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; sh = 0.06f; break;
                    default: r = 0.5f; g = 0.5f; b = 0.5f; break;
                }

                glUniform2f(uPosLoc, sx, stackY);
                glUniform2f(uScaleLoc, sw, sh);
                glUniform4f(uColorLoc, r, g, b, 1.0f);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                stackY += 0.045f;
            }

            // AKTIVNI SASTOJAK koji se pomera
            if (currentIngredient < ING_TOTAL) {
                float r, g, b;
                float iw = 0.18f, ih = 0.05f;

                switch (currentIngredient) {
                    case ING_DONJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; break;
                    case ING_PLJESKAVICA:   r = 0.4f; g = 0.22f; b = 0.12f; break;
                    case ING_KECAP:         r = 0.8f; g = 0.1f; b = 0.1f; iw = 0.06f; ih = 0.15f; break;
                    case ING_SENF:          r = 0.9f; g = 0.8f; b = 0.1f; iw = 0.06f; ih = 0.15f; break;
                    case ING_KRASTAVCICI:   r = 0.5f; g = 0.7f; b = 0.3f; break;
                    case ING_LUK:           r = 0.95f; g = 0.9f; b = 0.95f; break;
                    case ING_SALATA:        r = 0.3f; g = 0.8f; b = 0.3f; break;
                    case ING_SIR:           r = 1.0f; g = 0.85f; b = 0.2f; break;
                    case ING_PARADAJZ:      r = 0.9f; g = 0.2f; b = 0.15f; break;
                    case ING_GORNJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; ih = 0.07f; break;
                    default: r = 0.5f; g = 0.5f; b = 0.5f; break;
                }

                glUniform2f(uPosLoc, ingredientX, ingredientY);
                glUniform2f(uScaleLoc, iw, ih);
                glUniform4f(uColorLoc, r, g, b, 1.0f);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }
        }

        // --- STATE_FINISHED ---
        else if (currentState == STATE_FINISHED) {
            // Pozadina
            glClearColor(0.5f, 0.45f, 0.4f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // STO
            glUniform2f(uPosLoc, -1.0f, -1.0f);
            glUniform2f(uScaleLoc, 2.0f, 0.35f);
            glUniform4f(uColorLoc, 0.45f, 0.3f, 0.15f, 1.0f);
            glUniform1i(uUseTextureLoc, false);
            glUniform1f(uAlphaLoc, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // TANJIR
            glUniform2f(uPosLoc, plateX, plateY);
            glUniform2f(uScaleLoc, plateW, plateH);
            glUniform4f(uColorLoc, 0.9f, 0.9f, 0.85f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // GOTOV BURGER - pojednostavljeno
            float stackY = plateY + plateH;
            for (int i = 0; i < ING_TOTAL; i++) {
                float r, g, b;
                float sh = 0.04f;
                switch (i) {
                    case ING_DONJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; break;
                    case ING_PLJESKAVICA:   r = 0.4f; g = 0.22f; b = 0.12f; break;
                    case ING_KECAP:         r = 0.8f; g = 0.1f; b = 0.1f; sh = 0.02f; break;
                    case ING_SENF:          r = 0.9f; g = 0.8f; b = 0.1f; sh = 0.02f; break;
                    case ING_KRASTAVCICI:   r = 0.5f; g = 0.7f; b = 0.3f; break;
                    case ING_LUK:           r = 0.95f; g = 0.9f; b = 0.95f; break;
                    case ING_SALATA:        r = 0.3f; g = 0.8f; b = 0.3f; break;
                    case ING_SIR:           r = 1.0f; g = 0.85f; b = 0.2f; break;
                    case ING_PARADAJZ:      r = 0.9f; g = 0.2f; b = 0.15f; break;
                    case ING_GORNJA_ZEMICKA: r = 0.9f; g = 0.7f; b = 0.4f; sh = 0.06f; break;
                    default: r = 0.5f; g = 0.5f; b = 0.5f; break;
                }
                glUniform2f(uPosLoc, plateX + 0.02f, stackY);
                glUniform2f(uScaleLoc, plateW - 0.04f, sh);
                glUniform4f(uColorLoc, r, g, b, 1.0f);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                stackY += 0.045f;
            }

            // PRIJATNO! - zeleni banner
            glUniform2f(uPosLoc, -0.35f, 0.3f);
            glUniform2f(uScaleLoc, 0.7f, 0.15f);
            glUniform4f(uColorLoc, 0.2f, 0.7f, 0.3f, 0.95f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Beli tekst simulacija
            glUniform2f(uPosLoc, -0.25f, 0.35f);
            glUniform2f(uScaleLoc, 0.5f, 0.05f);
            glUniform4f(uColorLoc, 1.0f, 1.0f, 1.0f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // STUDENTSKI PODACI - uvek vidljivo
        if (texStudentInfo != 0) {
            // 1) dimenzije teksture u pixelima (ubaci prave vrednosti koje ima tvoj PNG)
            float imgW = 600.0f;   // <-- stavi prave vrednosti (width)
            float imgH = 300.0f;   // <-- stavi prave vrednosti (height)

            // 2) koliko prostora u NDC (x) želiš da zauzme kartica (npr 0.48)
            //    ovo je širina u NDC (NDC: -1..1)
            float desiredWidthNDC = 0.48f;

            // 3) računamo visinu u NDC tako da se očuva aspekt slike na ekranu
            //    formula izvedena iz mapiranja NDC -> pixeli:
            //    (scaleX / scaleY) * (screenWidth / screenHeight) = imgW / imgH
            float scaleX = desiredWidthNDC;
            float scaleY = desiredWidthNDC * (imgH / imgW) * ((float)wWidth / (float)wHeight);

            // 4) pozicija: bottom-left anchor se koristi (jer su ti vertexi 0..1)
            //    da bi karticu smestila u donji desni ugao izračunamo x tako da desna ivica bude malo
            //    udaljena od ruba (-1..1), tj. pos.x + scaleX = 1 - marginNDC
            float marginNDC_x = 0.02f; // udaljenost od desnog ruba (u NDC)
            float marginNDC_y = 0.02f; // udaljenost od donjeg ruba (u NDC)

            float posX = 1.0f - scaleX - marginNDC_x;   // bottom-left x tako da desna ivica bude margin od ruba
            float posY = -1.0f + marginNDC_y;           // bottom-left y blizu dna

            // 5) set uniforms i crtanje
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

        // Na kraju render dela, pre glfwSwapBuffers(window);

// PRILAGOĐENI KURSOR - dijagonalna spatula
        {
            float cursorX = mouseToNDC_X();
            float cursorY = mouseToNDC_Y();

            glUseProgram(shader);
            glBindVertexArray(VAO);
            glUniform1i(uUseTextureLoc, false);
            glUniform1f(uAlphaLoc, 1.0f);

            // === METALNI DEO - gornji levi ugao (širi kvadrat) ===
            // Senca
            glUniform2f(uPosLoc, cursorX - 0.05f, cursorY - 0.005f);
            glUniform2f(uScaleLoc, 0.07f, 0.05f);
            glUniform4f(uColorLoc, 0.3f, 0.3f, 0.35f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Glavni metalni deo
            glUniform2f(uPosLoc, cursorX - 0.048f, cursorY - 0.003f);
            glUniform2f(uScaleLoc, 0.07f, 0.05f);
            glUniform4f(uColorLoc, 0.78f, 0.78f, 0.82f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Sjaj
            glUniform2f(uPosLoc, cursorX - 0.04f, cursorY + 0.005f);
            glUniform2f(uScaleLoc, 0.04f, 0.03f);
            glUniform4f(uColorLoc, 0.95f, 0.95f, 0.98f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // === DIJAGONALNI SEGMENTI - pravimo vizuelnu dijagonalu ===
            // Segment 1 (prelaz metal->drška)
            glUniform2f(uPosLoc, cursorX + 0.015f, cursorY - 0.04f);
            glUniform2f(uScaleLoc, 0.025f, 0.03f);
            glUniform4f(uColorLoc, 0.5f, 0.32f, 0.22f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Segment 2
            glUniform2f(uPosLoc, cursorX + 0.03f, cursorY - 0.065f);
            glUniform2f(uScaleLoc, 0.025f, 0.03f);
            glUniform4f(uColorLoc, 0.45f, 0.28f, 0.18f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Segment 3
            glUniform2f(uPosLoc, cursorX + 0.045f, cursorY - 0.09f);
            glUniform2f(uScaleLoc, 0.025f, 0.03f);
            glUniform4f(uColorLoc, 0.45f, 0.28f, 0.18f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Segment 4 (glavni deo drške)
            glUniform2f(uPosLoc, cursorX + 0.06f, cursorY - 0.115f);
            glUniform2f(uScaleLoc, 0.025f, 0.04f);
            glUniform4f(uColorLoc, 0.45f, 0.28f, 0.18f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Segment 5 (nastavak)
            glUniform2f(uPosLoc, cursorX + 0.075f, cursorY - 0.15f);
            glUniform2f(uScaleLoc, 0.025f, 0.04f);
            glUniform4f(uColorLoc, 0.45f, 0.28f, 0.18f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Završetak drške (okruglo)
            glUniform2f(uPosLoc, cursorX + 0.085f, cursorY - 0.185f);
            glUniform2f(uScaleLoc, 0.03f, 0.025f);
            glUniform4f(uColorLoc, 0.35f, 0.22f, 0.14f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            // Svetli highlighter na drški (dijagonalna linija)
            glUniform2f(uPosLoc, cursorX + 0.018f, cursorY - 0.042f);
            glUniform2f(uScaleLoc, 0.01f, 0.02f);
            glUniform4f(uColorLoc, 0.6f, 0.42f, 0.32f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glUniform2f(uPosLoc, cursorX + 0.035f, cursorY - 0.07f);
            glUniform2f(uScaleLoc, 0.01f, 0.025f);
            glUniform4f(uColorLoc, 0.6f, 0.42f, 0.32f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glUniform2f(uPosLoc, cursorX + 0.062f, cursorY - 0.12f);
            glUniform2f(uScaleLoc, 0.01f, 0.035f);
            glUniform4f(uColorLoc, 0.6f, 0.42f, 0.32f, 1.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
        }

        //glBindVertexArray(0);
        //glUseProgram(0);

        glfwSwapBuffers(window);

        // FRAME LIMITER
        auto frameEnd = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = frameEnd - currentTime;
        if (elapsed.count() < FRAME_TIME) {
            std::this_thread::sleep_for(std::chrono::duration<double>(FRAME_TIME - elapsed.count()));
        }
    }

    // CLEANUP
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shader);
    if (texStudentInfo != 0) glDeleteTextures(1, &texStudentInfo);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}