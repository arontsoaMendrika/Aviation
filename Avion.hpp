#pragma once
#include <string>
#include <cmath>

enum class EtatAvion { EnVol, Atterri, Crashe };
enum class VueCamera { Arriere, Gauche, Droite };

struct Avion {
    double x = -7000.0; // Distance initiale négative par rapport au seuil
    double y = 10000.0; // Altitude initiale
    double z = 0.0;

    double vx = 0.0; // géré en m/s en interne
    double vy = 0.0;

    double ax = 0.0; // Freinage X cumulatif
    double ay = 0.0; // Freinage Y cumulatif

    double vitesseDecrochageKmh = 300.0;
    double gammaX_max = -40.0;
    double gammaY_max = -6.0;
    double pisteLongueur = 3000.0;

    EtatAvion etat = EtatAvion::EnVol;
    VueCamera vue = VueCamera::Arriere;
    std::string raisonCrash = "";

    double getVitesseSolKmh() const { return vx * 3.6; }

    // Système par paliers cumulatifs demandés par le TODO
    void ajusterAx(double direction) {
        if (etat == EtatAvion::Crashe) return;
        // direction: -1 pour freiner (-x), +1 pour relâcher (+x)
        ax += direction * 10.0; 
        if (ax < gammaX_max) ax = gammaX_max;
        if (ax > 0.0) ax = 0.0; // Pas d'accélération positive (uniquement freinage)
    }

    void ajusterAy(double direction) {
        if (etat != EtatAvion::EnVol) return;
        // direction: -1 pour descendre (-y), +1 pour remonter (+y)
        // Comme gammaY_max = -6, on avance par pas de 1 pour avoir du contrôle
        ay += direction * 1.0; 
        if (ay < gammaY_max) ay = gammaY_max;
        if (ay > 0.0) ay = 0.0;
    }

    void update(double dt) {
        if (etat == EtatAvion::Crashe) return;

        // Mise à jour des vitesses
        vx += ax * dt;
        vy += ay * dt;

        if (vx < 0.0) vx = 0.0;

        // Déplacement de l'avion
        x += vx * dt;
        y += vy * dt;

        // 1. Condition de décrochage en vol
        if (getVitesseSolKmh() < vitesseDecrochageKmh) {
            if (etat == EtatAvion::EnVol) {
                etat = EtatAvion::Crashe;
                raisonCrash = "DECROCHAGE (Vitesse < Seuil)";
                return;
            } else if (etat == EtatAvion::Atterri) {
                // Devient incontrôlable au sol sous la vitesse de décrochage
                ax = 0;
                ay = 0;
            }
        }

        // 2. Gestion du plan de contact sol (y <= 0)
        if (y <= 0.0) {
            y = 0.0;
            vy = 0.0;
            ay = 0.0;

            if (etat == EtatAvion::EnVol) {
                // L'avion touche le sol : est-il sur la piste [0, pisteLongueur] ?
                if (x >= 0.0 && x <= pisteLongueur) {
                    etat = EtatAvion::Atterri;
                    vitesseDecrochageKmh *= 0.5; // La vitesse de décrochage diminue de moitié
                } else {
                    etat = EtatAvion::Crashe;
                    if (x < 0.0) raisonCrash = "CRASH: Atterrissage TROP COURT !";
                    else raisonCrash = "CRASH: Atterrissage TROP LONG !";
                    return;
                }
            }
        }

        // 3. Sortie en bout de piste en roulant
        if (etat == EtatAvion::Atterri && x > pisteLongueur && vx > 0.1) {
            etat = EtatAvion::Crashe;
            raisonCrash = "CRASH: Sortie de piste !";
        }
    }
};