# Juegos de Tira

Índice de los juegos implementados en la tira de LEDs. Cada uno tiene su propio archivo con resumen, mecánica y controles.

Orden del menú en la tira:

| # | Juego | Resumen |
|---|---|---|
| 1 | [Left2Dead](Left2Dead.md) | Defensa compartida contra zombies: eliminalos antes de que crezcan o un zombie ataca y reinicia el juego |
| 2 | [TiraTenis](TiraTenis.md) | Tenis 1v1 sobre la tira, con saque, golpes y lob |
| 3 | [TiraZombies](TiraZombies.md) | Carrera de reflejos: cada player mueve su marcador con el control |
| 4 | [TiraColors](TiraColors.md) | Duelo de colores: cada player dispara su color contra el del rival |
| 5 | [TiraHero](TiraHero.md) | Guitar Hero en la tira, al ritmo de una melodía RTTTL |
| 6 | [TiraPaint](TiraPaint.md) | Lienzo compartido: cada player pinta la tira con el color de su control |
| 7 | [Metegol](Metegol.md) | Metegol 6v6, amarillo contra cian: cada botón patea a un grupo de jugadores |

## Controles físicos del joystick

Cada jugador tiene un control con:
- **Acelerómetro (inclinación)**: mueve/controla al jugador según el juego.
- **Botón 1 (BTN_FIRE)**: acción principal (disparar, golpear, pintar, según el juego).
- **Botón 2 (BTN_COLOR)**: acción secundaria (cambiar color, lob, etc., según el juego).

El significado exacto de la inclinación y de cada botón varía por juego — ver el detalle en cada archivo.
