# Avion:
## Technologies utilisées : 
    - backend: C++
    -Affichage aux choix
    -donnees dans  fichier: avion.json

## Explications:
## Affichages : vision 3D
    -On aure une piste
    -Timer(chrono) qui sera lancé dès que le jeu sera lancé et ne s'arrete pas meme si l'avion a deja atteri 
    -Tableau de bord : 
        .vitesse au sol (exemple: 900km/h), 
        .altitude(exemple: 10 000m),
        .distance par rapport à la piste(exemple: 7 000m) cela commencera en negatif -7000m,
        .vitesse de décrochage (exemple: 300km/h) : si la vitesse est en dessous de cela l'avion crash 
        => ces données seront récupérees dans le fichier de paramètrages de l'avion avion.json
        donc dans c'est dans ce fichier qu'on entre les données qui seront données par les jurys mais là on va juste prendre des exemples.
        => notre but c'est d'avoir un avion qui voles avec une vitesse et qu'on peut jouer avec sa vitesse(acceleration deceleration afin d'atteindre la piste sinon l'avion crash si en dehors piste ou vitesse de decrochage non atteind)
    -On aura un avion qui volera lors de la simulation
    -On aura un bouton pause pour figé la simulation ainsi que le chrono et un bouton play pour reprendre ou pour commencer le jeu.
    -On aura un fond ciel bleu 
    -On pourra aussi changé la vue de l'avion à gauche ou à droite ou par derrière (pour le derrière on devra voir la piste selon longueur de la piste car dès le départ la piste peut ne pas etre encore apercu sur l'ecran mais cela depend de la vitesse de l'avion) puisqu'on est dans un grand repère.
    -On aura 4 boutons de deceleration (freinage x et y) en -m/s^-2 : 2 boutons pour x et 2 pour y +x,-x,+y,-y 
    lorsqu'on clique cela va par pas exxemple on clique -x : -10m/s^-2 puis on clique de nouveau sur cela -20m/s^-2 puis on appuie sur +x : -10m/s^-2 car -20 avant etc, de meme pour y c'est cela qui va le diminuer ;du coup on aura 2 vitesses.
    -on aura une capacité de freinage maximum (gammaX et gammaY): exemple 30m/s de freinage car l'avion ne peut supporter la diminution il y aura des limites: on fixe par exemple gammaX=-40 et gammaY=-6 lorsque cela sera atteind on ne peut plus diminuer 
    => l'objectif est de voir si la distance de la piste est vraie sur l'axe des x
    -L'avion crash si elle n'est pas sur la piste 
    -Quand le pneu ou partie de l'avion atteind la partie de la piste lors de l'atterissage la vitesse de decrochage diminue mais l'avion ne s'arrete pas tout de suite on a encore la vitesse de l'avion qu'on peut diminuer et augmenter 
    -Quand la vitesse de decrochage est atteind elle n'est plus controlable 
    -Si on atteind le biut de piste alors que l'avion n'y est pas encore l'avion crash 

NB: À chaque changement de code on push sur github avec commande terminal 