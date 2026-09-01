# TiraPaint

## Resumen

Un lienzo compartido sobre la tira de LEDs: cada player controla un cursor que se mueve libremente (misma física que TiraZombies) y puede "pintar" la tira con un color que elige inclinando su control.

## Cómo se juega

- Cada player tiene un cursor que se desplaza por la tira inclinando el control, igual que en TiraZombies (acelera al inclinar, frena solo al soltar).
- El color del cursor se elige rolando el control de lado a lado: la inclinación va de -45° a +45° y recorre toda la gama de colores (arco iris completo) de un extremo al otro.
- Mientras el player se mueve sin apretar ningún botón, solo se desplaza: no deja marca en el fondo.
- Al mantener apretado cualquiera de los 2 botones, el cursor empieza a "pintar": todos los LEDs por los que pasa mientras tanto quedan pintados con el color que tenía el player en ese instante. Al soltar el botón, deja de pintar (pero se sigue moviendo).
- El lienzo es compartido entre ambos players y persiste en el tiempo: si un player pinta sobre una zona ya pintada por el otro, la sobrescribe.

## Controles

- **Inclinación del control (adelante/atrás)**: mueve el cursor del player a lo largo de la tira.
- **Rotación del control (rolido lateral, eje Z)**: elige el color actual del cursor, recorriendo toda la gama de colores entre -45° y +45° de inclinación lateral.
- **Botón 1 o Botón 2 (cualquiera de los dos)**: mantenido apretado, pinta la tira con el color actual mientras el cursor se mueve. Soltar deja de pintar.
