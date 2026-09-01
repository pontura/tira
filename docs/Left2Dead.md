# Left2Dead

## Resumen

No es un duelo entre players: es una defensa compartida contra zombies. Cada player se posiciona en cualquier punto de toda la tira inclinando su control (posición absoluta, sin inercia, con un filtro que suaviza el movimiento) y dispara sin límite para eliminar zombies que van apareciendo y creciendo cada vez más rápido. Los disparos nunca afectan al otro player. Si un zombie no se elimina a tiempo, ataca: la partida termina, se muestra cuántos zombies mató cada uno, y el juego arranca de cero.

## Cómo se juega

- La posición de cada player es una proyección directa de la inclinación del control (eje Y, igual que en TiraZombies) sobre toda la tira: a -45° o menos queda exactamente en un extremo, a +45° o más en el otro extremo, y todo lo intermedio se mapea linealmente entre ambos. Un filtro exponencial suaviza esa posición para que la mira no salte entre un LED y el de al lado por ruido del sensor.
- Cada player se dibuja con un único LED central, en su color: Player 1 amarillo, Player 2 cian.
- El disparo es instantáneo: no hay ninguna bala que viaje ni que se dibuje en la tira, y nunca impacta al otro player. Solo puede afectar a un zombie, y únicamente si en el momento exacto de disparar hay un zombie justo en la posición del player — si no, es un tiro que no hace nada. No hay munición ni recarga, pero sí un cooldown: cada player puede disparar como máximo 5 veces por segundo (una cada 0.2s).
- Cada disparo muestra un flash blanco centrado en la mira, que se va cerrando: 5 LEDs durante 0.1s, 3 LEDs durante 0.1s y 1 LED durante 0.2s.
- El display OLED muestra en vivo cuántos zombies mató cada player.

## Jugadores conectados

El sistema detecta cuando un control se prende o se apaga: si no manda ninguna señal durante 2 segundos, se lo considera desconectado.

- Al entrar a Left2Dead, si no hay ningún control prendido, el juego espera (la tira queda casi apagada, con un pulso suave en el centro) hasta que se conecte al menos uno.
- Con un solo player conectado, se dibuja únicamente su mira, y los zombies aparecen a la mitad de ritmo (el doble de intervalo entre apariciones) para que no sea imposible jugar en solitario.
- En cuanto se conecta el segundo player, empieza a dibujarse también y el ritmo de aparición de zombies vuelve al normal — todo esto sin reiniciar la partida en curso.

## Modos

El juego alterna entre dos modos de aparición de zombies. Cada modo deja aparecer gente nueva durante 40 segundos; agotado ese tiempo, no sale nadie más, y en cuanto se limpia la pantalla (se mata al último zombie que quedaba) se pasa al otro modo. En cada cambio de modo, la dificultad (el ritmo de spawn) se reajusta a mitad de camino entre el valor que quedó fijado en el cambio anterior y el valor actual — así cada salto suaviza un poco la dificultad acumulada, sin resetearla del todo.

- **Modo 1** (arranca así el juego): los zombies aparecen quietos, en una posición al azar, y no se mueven de ahí mientras crecen.
- **Modo 2**: los zombies aparecen desde los extremos de la tira (alternando izquierda/derecha) y viajan hacia un punto al azar en la zona central, con un movimiento que empieza rápido y se frena hasta casi detenerse — llegan a destino justo en el momento en que terminan de crecer y se ponen completamente rojos.

## Zombies

- Cada zombie que aparece acelera el ritmo: el intervalo de spawn arranca en 3.0s y baja 0.2s con cada zombie nuevo (2.8s, 2.6s...) hasta un mínimo de 0.5s (se reajusta a mitad de camino en cada cambio de modo, ver arriba). En modo 1 aparece en un LED al azar entre el 26 y el 118 (la zona central de la tira), alternando entre menor y mayor a la mitad; en modo 2 arranca desde un extremo de la tira y viaja hacia ese mismo rango central.
- Cada zombie crece un "momento" a la vez, en verde oscuro/verde/rojo, arrancando de 1 solo LED hasta un patrón de 19 LEDs. Ese crecimiento también se acelera junto con el ritmo de spawn, pero de forma más gradual y en un rango más angosto (1 segundo por momento al principio, hasta 0.5 segundos en el punto más difícil):
  1. `0` — 2. `G` — 3. `0G0` — 4. `0GGG0` — 5. `0GRGRG0` — 6. `0GGRGGRGG0` — 7. `0GGGRGGGRGGG0` — 8. `0GGGRRGGGRRGGG0` — 9. `0GGGGRRGGGRRGGGG0` — 10. `0GGGGRRRGGGRRRGGGG0` (tamaño máximo)
- Un disparo elimina a un zombie si en ese instante la posición del player cae en cualquier LED de su patrón (sea del color que sea), sumando un punto al conteo de ese player. Al eliminarlo, suena una explosión corta y salta una explosión de partículas verdes hacia ambos lados desde su posición, que se frenan y se apagan hasta desaparecer.
- Si la mira de un player queda dentro de un zombie, se "engancha" a él: en vez de su LED de color, aparecen 2 LEDs blancos 100% opacos en cada extremo del zombie (siguiéndolo mientras crece), mientras el punto que sigue la inclinación pasa a dibujarse en negro. Al salir de la zona del zombie, la mira vuelve a mostrarse como su único LED de color.
- Mientras dura el enganche, los LEDs verdes y verde oscuro del zombie también cambian de color como refuerzo extra: a naranja/naranja oscuro si es el player amarillo quien lo tiene en la mira, a azul/azul oscuro si es el cian.
- Mientras la mira sigue enganchada, suena un beep de alerta en el control de ese player, repitiéndose cada 0.35s, hasta que mate al zombie o la mira salga de su zona.
- Si dos zombies llegan a superponerse en la tira, el más viejo (el que apareció primero) siempre gana: se dibuja encima del más nuevo en los LEDs donde se pisan, y un disparo en esa zona impacta al más viejo primero.
- Al llegar al tamaño máximo (momento 10), el zombie entra en advertencia: se pinta completamente rojo (en vez de su patrón de colores) y el fondo de toda la tira se tiñe de un rojo muy tenue (~2%) como aviso. El juego sigue andando normalmente durante 0.5 segundos — todavía se lo puede matar con un disparo a tiempo, como a cualquier otro. Si pasan esos 0.5 segundos sin eliminarlo, recién ahí ataca (ver más abajo).

## Fin de partida: un zombie ataca

Si un zombie llega al tamaño máximo (momento 10) y pasan 0.5 segundos de advertencia sin ser eliminado, ataca y termina la partida en tres fases:

1. **Diseminación** (2s): el rojo opaco del zombie atacante, que arranca ocupando solo su propio ancho, se expande ease-in-out hacia ambos extremos hasta cubrir los 144 LEDs de la tira; todo el juego queda congelado mientras tanto (no aparecen ni crecen más zombies, no se puede disparar). Mientras se disemina, suena una secuencia de notas al azar con una caída general de octava (una "bajada" descendente pero con cada nota al azar dentro de ese rango).
2. **Conteo**: se apagan todos los LEDs y arranca la revelación de cuántos zombies mató cada uno — un LED blanco por zombie matado, uno a la vez, alternando player 1 y player 2, cada uno creciendo desde su propio extremo hacia el centro (si a alguno se le acaban los suyos antes, sigue el otro solo hasta terminar).
3. **Pausa final** (1s): con el conteo completo en pantalla, espera un segundo y reinicia el juego desde cero (zombies, dificultad y conteo de vuelta a su estado inicial).

## Controles

- **Inclinación del control (eje Y)**: define la posición absoluta del player en toda la tira. -45° o menos = un extremo; +45° o más = el otro extremo.
- **Botón 1 o Botón 2 (cualquiera de los dos)**: dispara, eliminando al zombie que esté justo en la posición del player en ese instante (si no hay ninguno, no pasa nada). Máximo 5 disparos por segundo (uno cada 0.2s).
