# TiraColors

## Resumen

Duelo de colores 1 contra 1 (antes llamado TiraGestures): desde cada extremo de la tira van apareciendo bloques de color hacia el centro. Cada player dispara balas de su color elegido; si la bala impacta un bloque del mismo color, lo destruye y empuja el frente hacia el rival; si impacta uno de otro color, recibe una penalidad.

## Cómo se juega

- Los bloques de color se generan alternadamente desde cada extremo y avanzan hacia el centro, donde está el "spawner" (2 LEDs blancos con un hueco negro en el medio).
- Cada player elige un color para su próxima bala (arranca con 3 colores disponibles; se suman más colores con el tiempo, hasta 6).
- Al disparar, la bala viaja desde el spawner hacia el extremo del player. Si impacta el segmento contiguo de bloques y coincide en color, explota (con partículas) y el frente de bloques se corre hacia el lado del rival — ganando terreno. Colores más "avanzados" en el ciclo destruyen más de un segmento de una y empujan más lejos.
- Si la bala impacta un color distinto, no destruye nada y el player recibe una penalidad: se le agregan bloques del color equivocado justo en su propio extremo, achicando su margen.
- Pierde el player cuyo extremo de la tira se llena por completo de bloques (el frente le llega hasta el final). Al terminar, hay una animación de barrido y "game over", y el juego reinicia solo.
- El disparo no se hace con un botón: se hace con un gesto físico (sacudida brusca de muñeca), heredado de la mecánica original de "TiraGestures".

## Controles

- **Gesto de muñeca (sacudida brusca del control)**: dispara una bala del color actualmente seleccionado. El color de esa bala se auto-cicla al siguiente después de cada disparo.
- **Botón 1 (FIRE)**: cambia al color siguiente en el ciclo.
- **Botón 2 (COLOR)**: cambia al color anterior en el ciclo.
- La inclinación sostenida del control no se usa; solo importa el movimiento brusco para disparar.
