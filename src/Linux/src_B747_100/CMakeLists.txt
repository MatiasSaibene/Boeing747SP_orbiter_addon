set(CMAKE_CXX_STANDARD 20)
cmake_minimum_required(VERSION 3.16)
project(Boeing747100)

# Directorio que contiene las bibliotecas
set(LIBRARY_DIRECTORY "$ENV{HOME}/orbiter_test/Orbiter/Orbitersdk/lib")

file(GLOB LIBRARIES "${LIBRARY_DIRECTORY}/*.a")

# Definiciones de los archivos de código fuente
set(SOURCE_FILES
    Boeing747_100.cpp
)

# Definiciones de los archivos de encabezado
set(HEADER_FILES
    Boeing747_100.h
    747cockpitdefinitions.h
    747FCdefinitions.h
    747100definitions.h
)

# Agregar los archivos de código fuente y encabezado al proyecto
add_library(${PROJECT_NAME} SHARED
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

# Especificar directorios de inclusión
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "$ENV{HOME}/orbiter_test/Orbiter/Orbitersdk/include"
)

# Enlazar con todas las bibliotecas encontradas en el directorio
target_link_libraries(Boeing747100 ${LIBRARIES})

# Opciones de compilación
add_compile_options(-fPIC)
