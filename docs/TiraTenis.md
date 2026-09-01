# TiraTenis

## Resumen

Un tenis 1 contra 1 jugado sobre la tira de LEDs: cada player controla una "paleta" en su mitad de la tira, con una "red" fija en el centro. La pelota rebota entre ambos lados hasta que alguien falla el golpe o deja que la pelota pase.

## Cómo se juega

- La tira se divide en dos mitades por una red (LED gris apagado en el centro).
- Cada player mueve su paleta (2 LEDs blancos) dentro de su mitad.
- Cuando le toca sacar, el player debe golpear en el momento justo mientras la pelota (amarilla) va y viene desde su paleta.
- Durante el rally, si la pelota entra en la zona de alcance de la paleta, el player puede golpearla de vuelta. Si no llega a tiempo o golpea fuera de zona, pierde el punto.
- Cada golpe muestra un barrido de color (azul = golpe normal, verde = lob) desde la paleta hacia el centro; si el barrido "toca" la pelota a tiempo, el golpe es válido.
- Gana la partida el primer player en llegar a 5 puntos. El puntaje se ve como LEDs naranjas tenues que crecen desde cada extremo.
- Al ganar un punto se resetea el saque; al ganar el partido hay una animación de cierre y el juego reinicia solo.

## Controles

- **Inclinación del control**: mueve la paleta del player a lo largo de su mitad de la tira (más inclinación = más velocidad).
- **Botón 1 (FIRE)**: golpe normal / saque.
- **Botón 2 (COLOR)**: golpe en lob (más alto y lento, trayectoria en verde) — solo tiene efecto durante el rally, no en el saque.
