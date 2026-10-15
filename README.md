# KikoStart GRO

Plugin de **sidechain / ducking multibanda** (VST3 para Windows, macOS y Linux; AU en macOS; app Standalone).

## Características

| Función | Cómo funciona |
|---|---|
| **Ducking multibanda** | Botón **BAND** + slider inferior (30 Hz – 1 kHz). Solo se aplica el ducking por debajo del crossover (Linkwitz-Riley 4º orden); medios y agudos quedan intactos. El botón de auriculares (**listen**) deja oír solo la banda afectada. |
| **Visualizador** | Muestra la forma de onda de la entrada en un ciclo, la curva de volumen (amarillo) y el cabezal de reproducción. |
| **Disparo** | **SYNC** (tempo/posición del host), **MIDI** (cada nota reinicia la curva) y **AUDIO** (detecta el bombo en la entrada sidechain; ajusta el umbral con el slider **LEVEL**). |
| **16 curvas** | Cuadrícula inferior. Arrastra el tirador `|||` del visualizador para cambiar la **longitud**, arrastra el resto de la pantalla (o usa `<<  <  >  >>`) para **desplazar** la curva. |
| **Rate** | 1/8, 1/4, 1/2, 1/1 (longitud del ciclo en pulsos). |
| **Mix** | Dry/Wet con el knob central. |

> Si no hay sidechain conectado, el modo AUDIO analiza la propia entrada (útil para probar el Standalone).
> MIDI → plugin: Ableton Live no enruta MIDI a efectos de audio; usa Reaper, FL Studio, Bitwig, Cubase, Logic (AU), etc.

## Compilar

Requisitos: CMake ≥ 3.22, compilador C++17 y Git. JUCE se descarga solo (FetchContent).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Los binarios quedan en `build/KikoStartGRO_artefacts/...` y, por defecto, se copian a tu carpeta de plugins
(`-DKG_COPY_AFTER_BUILD=OFF` para desactivarlo).

**Linux** — dependencias (Debian/Ubuntu):

```bash
sudo apt install build-essential cmake git libasound2-dev libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev \
  libxrender-dev libgl1-mesa-dev
```

**macOS** — se genera binario universal (arm64 + x86_64). Para plugins sin notarizar:
`codesign --force --deep -s - "KikoStart GRO.vst3"` y, si el sistema lo bloquea, `xattr -cr "KikoStart GRO.vst3"`.

**Sin tener las 3 máquinas:** sube la carpeta a GitHub; el workflow `.github/workflows/build.yml` compila en
Linux, Windows y macOS y deja los plugins como artefactos descargables.

## Licencia / distribución

JUCE se usa bajo AGPLv3 o licencia comercial (https://juce.com/get-juce). Si vas a vender el plugin
sin publicar el código, necesitas una licencia comercial de JUCE. El nombre, gráficos y curvas de este
proyecto son originales; no uses marcas de terceros en tu distribución.
