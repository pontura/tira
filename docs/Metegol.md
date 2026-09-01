# Metegol

## Resumen

Metegol clásico llevado a la tira: 6 jugadores amarillos contra 6 cian, cada uno representado por un único LED sobre un fondo verde casi negro, más una pelota (LED blanco) que rueda con fricción y que solo se puede patear en el momento justo. Todavía no hay marcador ni arcos que sumen gol — eso queda para más adelante.

## Formación

- El arquero de cada equipo está fijo a 5 LEDs de su punta de la tira (amarillo cerca del LED 0, cian cerca del último LED).
- Los 10 jugadores de campo restantes se reparten parejos entre los dos arqueros así: desde cada extremo se ve arquero, atacante rival, defensor propio, atacante rival, defensor propio, atacante rival — y del otro lado, mirado en espejo, atacante propio, defensor rival, atacante propio, defensor rival, atacante propio. De esta forma cada defensor queda marcando de cerca a los delanteros rivales que atacan su propio arco.

## Cómo se juega

- Cada jugador tiene 2 estados: **reposo** (color tenue, ~5% de brillo) y **pateando** (color 100% opaco). Arranca en reposo.
- Al apretar el botón que le corresponde, hace una secuencia de 3 movimientos, siempre sobre su propio LED de origen ("home"):
  1. **Amague**: retrocede 5 LEDs hacia su propio arco, rápido al arrancar y frenando (sigue en reposo/tenue).
  2. **Patada**: se pone 100% opaco y avanza 10 LEDs hacia el arco rival (lento → rápido → lento), terminando 5 LEDs pasado su posición original.
  3. **Vuelta**: vuelve a ponerse tenue y regresa a su posición original, lento al arrancar y acelerando.
- Mientras un jugador está en amague, patada o vuelta, ignora cualquier nuevo botón hasta terminar el ciclo completo y volver a reposo.

## La pelota

- Al arrancar la partida, la pelota (LED blanco) aparece en el centro de la tira y se lanza una sola vez, a una velocidad al azar entre 1/4 y 1/2 de la velocidad máxima de saque: la primera partida hacia la derecha, la siguiente hacia la izquierda, alternando cada vez que se entra al juego.
- Tiene fricción: arranca fuerte y se va frenando de a poco. Si nadie la toca y queda completamente quieta en algún punto de la tira (sin llegar a ningún extremo), vuelve a salir sola desde el centro en una dirección al azar, otra vez a una velocidad al azar entre 1/4 y 1/2 de la máxima.
- Un jugador solo la patea si en ese instante está en el tramo de **patada** (100% opaco), todavía no le pegó en esa misma animación (un jugador nunca golpea la pelota dos veces en la misma patada) y su LED coincide con el de la pelota — en reposo, amague o vuelta la pelota lo ignora y pasa de largo. Al patearla, sale disparada hacia el arco rival de ese jugador (el lado contrario al de su propio arquero).
- La fuerza del golpe depende de dónde está la pelota respecto al home del jugador en el momento del contacto: si lo agarra justo en su LED de origen sale lo más fuerte posible, y cuanto más cerca del límite de su recorrido de patada (±5 LEDs del home) la golpee, más despacio sale.
- Si la pelota ya viaja hacia el mismo lado al que patea el jugador, esa fuerza se suma a la velocidad que ya traía (nunca se la resta); si venía detenida o yendo para el otro lado, la patada le fija esa velocidad directamente.
- Cada patada suena en el master con uno de 5 golpes posibles, de más fuerte a más débil, según esa misma distancia al home del jugador.

## Gol

- Si la pelota llega a cualquiera de los dos extremos de la tira, es gol para el equipo que atacaba ese lado y el juego se congela: ningún botón mueve a nadie hasta que termine el festejo.
- El equipo que anotó se va agrandando desde el home de cada uno de sus jugadores hasta cubrir toda la tira; mientras tanto, los LEDs del equipo que lo recibió se apagan en fade (lo que no llegue a taparse por el avance del otro equipo, se apaga solo).
- Suena una melodía de festejo en el master durante los ~3 segundos que dura toda la celebración.
- Al terminar la música, el juego se reinicia (todos los jugadores vuelven a su home, en reposo) y, 1 segundo después, la pelota vuelve a salir del centro — pero esta vez, a diferencia del saque normal, siempre hacia el mismo extremo que acaba de recibir el gol, y a una velocidad fija (sin azar) igual al promedio del rango que usa el saque normal (1/4 y 1/2 de la máxima).

## Controles

- **Botón 1**: patean juntos el arquero y los 2 defensores del equipo de ese control.
- **Botón 2**: patean juntos los 3 delanteros del equipo de ese control.
- La inclinación del control no se usa en este juego.
