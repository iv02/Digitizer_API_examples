# Список необходимых библиотек

## Qt 6.4.x
### Core 
### Network 
### StateMachine
## gRPC 1.68.0

## Настройка путей

По умолчанию зависимости ищутся в каталогах `libs` и `digitizer-api` внутри
репозитория. Внешние каталоги можно указать при конфигурации:

```bash
cmake -S . -B build \
  -DDIGITIZER_API_EXAMPLES_PREBUILD_LIBS_ROOT=/path/to/libs \
  -DDIGITIZER_API_EXAMPLES_API_ROOT=/path/to/platform/api/install
```