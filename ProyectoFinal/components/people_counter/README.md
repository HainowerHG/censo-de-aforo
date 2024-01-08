Componente Memoria Flash 

Este componente de memoria flash para las placas ESP32. Permite escribir, leer y consultar datos almacenados en memoria flash.

Uso
Inicialización
Para utilizar este componente, sigue estos pasos:

1. Incluye flash.h en tu proyecto.
2. Inicializa el componente llamando a flash_init() en tu función app_main() o donde sea apropiado.
### `void flash_init(flash_t* flash, uint8_t* buffer, size_t size)`

- `flash`: Un puntero a una estructura `flash_t` que representa la memoria flash.
- `buffer`: Un puntero al buffer de memoria flash que se utilizará.
- `size`: El tamaño total del buffer de memoria flash.

3. Escribe datos en la memoria flash simulada.

### `esp_err_t writeToFlash(flash_t* flash, void* data, size_t size)`

- `flash`: Un puntero a una estructura `flash_t` que representa la memoria flash simulada.
- `data`: Un puntero a los datos que se desean escribir.
- `size`: El tamaño de los datos a escribir.

4. Lee datos desde la memoria flash.

### `void* readFromFlash(flash_t* flash, size_t size)`

- `flash`: Un puntero a una estructura `flash_t` que representa la memoria flash simulada.
- `size`: El tamaño de los datos que se desean leer.
