# TiraZombies

## Resumen

El juego más simple de la tira: cada player controla un marcador de color que se desplaza libremente por toda la tira, con física de aceleración y fricción. Sin puntaje ni objetivo formal — pensado como base de movimiento (otros juegos, como TiraPaint, reutilizan esta misma física).

## Cómo se juega

- Player 1 (rojo) y Player 2 (azul) arrancan cerca del centro de la tira, cada uno representado por 2 LEDs de su color.
- Inclinando el control, el marcador acelera en esa dirección; al soltar (control plano), frena solo por fricción.
- La tira actúa como un loop: si un marcador llega a un extremo, reaparece del otro lado.
- No hay colisiones, puntaje ni fin de partida — es libre movimiento continuo.

## Controles

- **Inclinación del control**: única entrada del juego. Inclinar hacia un lado acelera el marcador en esa dirección; a mayor inclinación, mayor velocidad (con un tope máximo). Sin inclinación, el marcador frena gradualmente hasta detenerse.
- Los botones no tienen función en este juego.
