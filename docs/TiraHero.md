# TiraHero

## Resumen

Versión "Guitar Hero" de la tira: al ritmo de una melodía (formato RTTTL), bloques de nota salen del centro hacia cada extremo y cada player debe presionar su botón justo cuando el bloque llega a su base.

## Cómo se juega

- Las notas de la canción se reparten alternadamente: las notas pares viajan hacia el Player 2 (derecha) y las impares hacia el Player 1 (izquierda), coloreadas en amarillo/violeta.
- Cada player tiene una "base" fija a cierta distancia del centro. Cuando el bloque de nota llega a la base, hay una ventana de tolerancia para presionar el botón y acertar (la base se pone verde) o fallar (roja).
- Si se falla una nota (no se presiona a tiempo, o se mantiene el botón apretado de más), se suma una penalización, mostrada como LEDs rojos creciendo desde el centro hacia el player.
- Al acumular 3 penalizaciones, el player queda eliminado: su mitad de la tira se pone roja y las notas le siguen pasando pero apagadas, sin poder jugar. Si ambos players quedan eliminados, es game over y el juego reinicia.
- Al llegar al final de la canción, cada player que perdió alguna vida recupera una; si algún player había quedado eliminado, revive con una sola vida restante (2 LEDs rojos). Después de 2 segundos de silencio, arranca la siguiente canción de la lista (son 11 niveles, ordenados de más lenta a más rápida: Stairway to Heaven, Los Pitufos, Barbie Girl, Bolero, Los Locos Addams, Los Simpson, Star Wars, Superman, Halloween, Final Countdown y Back to the Future), y al llegar a la última vuelve a la primera.

## Controles

- **Botón 1 o 2 (cualquiera de los dos, según a qué player pertenece el control)**: presionar cuando el bloque de nota llega a la base propia, y soltar cuando termina la nota. Mantenerlo apretado sin motivo (sin nota en la base) o de más tiempo del debido también penaliza.
- La inclinación del control no se usa en este juego.
