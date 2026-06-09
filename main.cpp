#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "nlohmann/json.hpp"
#include "Avion.hpp"

using json = nlohmann::json;

struct Nuage {
    float px, py, pz;
    float tailleX, tailleY, tailleZ;
};

void appliquerLookAt(double eyex, double eyey, double eyez, 
                     double centerx, double centery, double centerz, 
                     double upx, double upy, double upz) {
    double f[3] = { centerx - eyex, centery - eyey, centerz - eyez };
    double f_len = std::sqrt(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
    if (f_len > 0.0) { f[0]/=f_len; f[1]/=f_len; f[2]/=f_len; }
    double u[3] = { upx, upy, upz };
    double u_len = std::sqrt(u[0]*u[0] + u[1]*u[1] + u[2]*u[2]);
    if (u_len > 0.0) { u[0]/=u_len; u[1]/=u_len; u[2]/=u_len; }
    double s[3] = { f[1]*u[2] - f[2]*u[1], f[2]*u[0] - f[0]*u[2], f[0]*u[1] - f[1]*u[0] };
    double s_len = std::sqrt(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
    if (s_len > 0.0) { s[0]/=s_len; s[1]/=s_len; s[2]/=s_len; }
    u[0] = s[1]*f[2] - s[2]*f[1]; u[1] = s[2]*f[0] - s[0]*f[2]; u[2] = s[0]*f[1] - s[1]*f[0];
    double m[16] = { s[0], u[0], -f[0], 0.0, s[1], u[1], -f[1], 0.0, s[2], u[2], -f[2], 0.0, 0.0, 0.0, 0.0, 1.0 };
    glMultMatrixd(m);
    glTranslated(-eyex, -eyey, -eyez);
}

void dessinerCubeLowPoly(float tx, float ty, float tz) {
    glBegin(GL_QUADS);
    glVertex3f(-tx, ty, -tz);  glVertex3f(tx, ty, -tz);  glVertex3f(tx, ty, tz);  glVertex3f(-tx, ty, tz);
    glVertex3f(-tx, -ty, -tz); glVertex3f(tx, -ty, -tz); glVertex3f(tx, -ty, tz); glVertex3f(-tx, -ty, tz);
    glVertex3f(-tx, -ty, tz);  glVertex3f(tx, -ty, tz);   glVertex3f(tx, ty, tz);  glVertex3f(-tx, ty, tz);
    glVertex3f(-tx, -ty, -tz); glVertex3f(tx, -ty, -tz);  glVertex3f(tx, ty, -tz); glVertex3f(-tx, ty, -tz);
    glEnd();
}

bool chargerParametres(Avion& avion, const std::string& fichier) {
    std::ifstream ifs(fichier);
    if (!ifs.is_open()) return false;
    json j; ifs >> j;
    avion.vx = j["vitesse_initiale_kmh"].get<double>() / 3.6;
    avion.y = j["altitude_initiale_m"].get<double>();
    avion.x = j["distance_piste_initiale_m"].get<double>();
    avion.vitesseDecrochageKmh = j["vitesse_decrochage_initiale_kmh"].get<double>();
    avion.gammaX_max = j["gammaX_max"].get<double>();
    avion.gammaY_max = j["gammaY_max"].get<double>();
    avion.pisteLongueur = j["piste_longueur_m"].get<double>();
    return true;
}

bool chargerPolice(sf::Font& font) {
    const char* chemins[] = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Helvetica.ttf",
        "/System/Library/Fonts/Supplemental/Courier New.ttf"
    };

    for (const char* chemin : chemins) {
        if (font.loadFromFile(chemin)) {
            return true;
        }
    }

    return false;
}

int main() {
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Aviation Simulator 3D", sf::Style::Default, sf::ContextSettings(24));
    window.setFramerateLimit(60);

    Avion avion;
    if (!chargerParametres(avion, "avion.json")) return -1;

    sf::Clock clockDelta;
    float chronoGlobal = 0.0f;
    bool estEnPause = true;

    // Chargement robuste de la police pour Mac M1/M2/Intel
    sf::Font font;
    bool policeChargee = chargerPolice(font);

    // Nuages placés le long du parcours mondial (de -8000m à +5000m)
    std::vector<Nuage> nuages;
    for (int i = 0; i < 40; ++i) {
        nuages.push_back({ (float)(-10000 + rand() % 20000), (float)(3000 + rand() % 4000), (float)(-4000 + rand() % 8000),
                           (float)(200 + rand() % 150), (float)(60 + rand() % 40), (float)(150 + rand() % 100) });
    }

    glEnable(GL_DEPTH_TEST);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::P)    estEnPause = !estEnPause;
                if (event.key.code == sf::Keyboard::Num1) avion.vue = VueCamera::Arriere;
                if (event.key.code == sf::Keyboard::Num2) avion.vue = VueCamera::Gauche;
                if (event.key.code == sf::Keyboard::Num3) avion.vue = VueCamera::Droite;

                // Système de modification par paliers tel qu'exigé par le TODO
                if (event.key.code == sf::Keyboard::Left)  avion.ajusterAx(-1.0); // Freiner (-x)
                if (event.key.code == sf::Keyboard::Right) avion.ajusterAx(1.0);  // Relâcher freins (+x)
                if (event.key.code == sf::Keyboard::Down)  avion.ajusterAy(-1.0); // Descendre (-y)
                if (event.key.code == sf::Keyboard::Up)    avion.ajusterAy(1.0);  // Atténuer descente (+y)
            }
        }

        float dt = clockDelta.restart().asSeconds();
        if (!estEnPause) {
            chronoGlobal += dt;
            avion.update(dt);
        }

        // Ciel couleur bleu cyan d'origine
        glClearColor(0.24f, 0.70f, 0.78f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double ratio = 1280.0 / 720.0;
        glFrustum(-ratio, ratio, -1.0, 1.0, 2.0, 150000.0); // Z-Far très lointain pour voir venir la piste

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Caméra embarquée fixée sur l'avion (La piste défile par rapport à l'avion)
        if (avion.vue == VueCamera::Arriere) {
            appliquerLookAt(avion.x - 400.0, avion.y + 160.0, avion.z, 
                            avion.x + 600.0, avion.y - 60.0, avion.z, 
                            0.0, 1.0, 0.0);
        } else if (avion.vue == VueCamera::Gauche) {
            appliquerLookAt(avion.x, avion.y + 120.0, avion.z - 500.0, avion.x, avion.y, avion.z, 0.0, 1.0, 0.0);
        } else if (avion.vue == VueCamera::Droite) {
            appliquerLookAt(avion.x, avion.y + 120.0, avion.z + 500.0, avion.x, avion.y, avion.z, 0.0, 1.0, 0.0);
        }

        // DESSIN DES NUAGES STATIQUES DANS LE REPERE MONDE
        glColor3f(0.88f, 0.94f, 0.96f);
        for (const auto& n : nuages) {
            glPushMatrix();
            glTranslatef(n.px, n.py, n.pz);
            dessinerCubeLowPoly(n.tailleX, n.tailleY, n.tailleZ);
            glPopMatrix();
        }

        // GRILLE BLEU CYBERPUNK (Sert de repère de défilement visuel global)
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glColor3f(0.1f, 0.8f, 0.85f);
        for (double i = -50000; i <= 50000; i += 1500) {
            glVertex3f(i, -5.0f, -40000.0f); glVertex3f(i, -5.0f, 40000.0f);
            glVertex3f(-50000.0f, -5.0f, i); glVertex3f(50000.0f, -5.0f, i);
        }
        glEnd();

        // =================================================================
        // RENDU DE LA PISTE EXACTEMENT BALISÉE DE 0 À LongueurPiste
        // =================================================================
        double pisteLargeur = 120.0;

        // Bitume (N'existe que de X = 0 à X = pisteLongueur)
        glBegin(GL_QUADS);
        glColor3f(0.14f, 0.14f, 0.16f);
        glVertex3f(0.0f, 0.0f, -pisteLargeur);
        glVertex3f(avion.pisteLongueur, 0.0f, -pisteLargeur);
        glVertex3f(avion.pisteLongueur, 0.0f, pisteLargeur);
        glVertex3f(0.0f, 0.0f, pisteLargeur);
        glEnd();

        // Lignes blanches latérales de démarcation de la piste
        glLineWidth(4.0f);
        glBegin(GL_LINES);
        glColor3f(0.95f, 0.95f, 0.95f);
        glVertex3f(0.0f, 0.5f, -pisteLargeur + 3.0); glVertex3f(avion.pisteLongueur, 0.5f, -pisteLargeur + 3.0);
        glVertex3f(0.0f, 0.5f, pisteLargeur - 3.0);  glVertex3f(avion.pisteLongueur, 0.5f, pisteLargeur - 3.0);
        glEnd();

        // Ligne pointillée axiale centrale d'aéroport
        glBegin(GL_QUADS);
        glColor3f(0.9f, 0.9f, 0.9f);
        for (double d = 0; d < avion.pisteLongueur; d += 120) {
            glVertex3f(d, 0.6f, -2.5f);
            glVertex3f(d + 60.0, 0.6f, -2.5f);
            glVertex3f(d + 60.0, 0.6f, 2.5f);
            glVertex3f(d, 0.6f, 2.5f);
        }
        glEnd();

        // Zébrages réglementaires de seuil (Threshold markings) aux deux extrémités
        glBegin(GL_QUADS);
        glColor3f(0.95f, 0.95f, 0.95f);
        for (double w = -pisteLargeur + 15.0; w < pisteLargeur - 15.0; w += 20.0) {
            // Seuil d'entrée (X = 0m à X = 100m)
            glVertex3f(0.0f, 0.7f, w);
            glVertex3f(100.0f, 0.7f, w);
            glVertex3f(100.0f, 0.7f, w + 8.0f);
            glVertex3f(0.0f, 0.7f, w + 8.0f);
            
            // Seuil de fin de piste
            glVertex3f(avion.pisteLongueur - 100.0f, 0.7f, w);
            glVertex3f(avion.pisteLongueur, 0.7f, w);
            glVertex3f(avion.pisteLongueur, 0.7f, w + 8.0f);
            glVertex3f(avion.pisteLongueur - 100.0f, 0.7f, w + 8.0f);
        }
        glEnd();

        // RENDU DE L'AVION
        glPushMatrix();
        glTranslated(avion.x, avion.y, avion.z);
        glBegin(GL_TRIANGLES);
        glColor3f(0.92f, 0.94f, 0.96f);
        glVertex3f(30.0f, 0.0f, 0.0f); glVertex3f(-25.0f, -2.5f, -4.0f); glVertex3f(-25.0f, -2.5f, 4.0f);
        glColor3f(0.75f, 0.12f, 0.15f);
        glVertex3f(5.0f, -0.2f, 0.0f); glVertex3f(-20.0f, -0.2f, -35.0f); glVertex3f(-20.0f, -0.2f, 35.0f);
        glEnd();
        glPopMatrix();

        // RENDU HUD 2D
        window.pushGLStates();

        if (policeChargee) {
            sf::RectangleShape hudBg(sf::Vector2f(340.0f, 580.0f));
            hudBg.setPosition(910.0f, 40.0f);
            hudBg.setFillColor(sf::Color(10, 40, 40, 180)); 
            hudBg.setOutlineThickness(2.0f);
            hudBg.setOutlineColor(sf::Color(50, 180, 150));
            window.draw(hudBg);

            std::stringstream ssMetrics;
            int min = (int)chronoGlobal / 60;
            int sec = (int)chronoGlobal % 60;

            ssMetrics << "\n   TIMER\n"
                     << "     " << min << ":" << (sec < 10 ? "0" : "") << sec << "\n\n"
                     << " -----------------------------------\n\n"
                     << "   VITESSE AU SOL\n"
                     << "     " << std::fixed << std::setprecision(0) << avion.getVitesseSolKmh() << " km/h\n\n"
                     << " -----------------------------------\n\n"
                     << "   ALTITUDE\n"
                     << "     " << std::fixed << std::setprecision(0) << avion.y << " m\n\n"
                     << " -----------------------------------\n\n"
                     << "   DISTANCE PISTE\n"
                     << "     " << std::fixed << std::setprecision(0) << avion.x << " m\n\n"
                     << " -----------------------------------\n\n"
                     << "   VITESSE DECR\n"
                     << "     " << std::fixed << std::setprecision(0) << avion.vitesseDecrochageKmh << " km/h\n\n"
                     << " -----------------------------------\n"
                     << "   FREIN X: " << (int)avion.ax << " m/s^2\n"
                     << "   FREIN Y: " << (int)avion.ay << " m/s^2";

            if (avion.etat == EtatAvion::Crashe) {
                ssMetrics << "\n\n   !! " << avion.raisonCrash << " !!";
            } else if (avion.etat == EtatAvion::Atterri) {
                ssMetrics << "\n\n   SUCCESS ! ATTERRI";
            } else if (estEnPause) {
                ssMetrics << "\n\n   [ PAUSE - APPUYER SUR P ]";
            }

            sf::Text metricsText(ssMetrics.str(), font, 18);
            metricsText.setPosition(930.0f, 50.0f);
            metricsText.setFillColor(sf::Color(100, 255, 200)); 
            window.draw(metricsText);
        }

        auto dessinerBoutonHUD = [&](std::string txt, float x, float y, bool actif) {
            sf::RectangleShape box(sf::Vector2f(120.0f, 50.0f));
            box.setPosition(x, y);
            box.setFillColor(sf::Color(15, 45, 45, 200));
            box.setOutlineThickness(1.5f);
            box.setOutlineColor(actif ? sf::Color(100, 255, 200) : sf::Color(40, 100, 90));
            window.draw(box);

            sf::Text label(txt, font, 18);
            label.setPosition(x + 35.0f, y + 12.0f);
            label.setFillColor(actif ? sf::Color(100, 255, 200) : sf::Color(40, 140, 120));
            window.draw(label);
        };

        // Les boutons s'allument si un palier de freinage est actif
        dessinerBoutonHUD("-X", 510.0f, 580.0f, avion.ax < 0);
        dessinerBoutonHUD("+X", 650.0f, 580.0f, avion.ax == 0);
        dessinerBoutonHUD("-Y", 510.0f, 640.0f, avion.ay < 0);
        dessinerBoutonHUD("+Y", 650.0f, 640.0f, avion.ay == 0);

        window.popGLStates();
        window.display();
    }

    return 0;
}