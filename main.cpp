// Nombre: Martin Natera
// Cedula: 30445341
// Materia: Arquitectura del computador

// Inclusion de librerias estandar
#include <iostream>       // Para entrada/salida por consola
#include <fstream>        // Para manejo de archivos
#include <string>         // Para uso de cadenas de texto
#include <algorithm>      // Para algoritmos como transform, sort, etc.
#include <cctype>         // Para funciones de caracteres como tolower, isalpha
#include <sys/stat.h>     // Para operaciones del sistema de archivos (stat)
#include <ctime>          // Para manejo de fechas y horas
#include <vector>         // Para contenedor vector

// Librerias de terceros 
#include <boost/multi_index_container.hpp>      // Contenedor con multiples indices 
#include <boost/multi_index/ordered_index.hpp>  // Indices ordenados 
#include <boost/multi_index/member.hpp>         // Para acceso a miembros de struct
#include <boost/multi_index/sequenced_index.hpp> // Indice secuencial
#include <leveldb/db.h>                         // Base de datos clave-valor embedida


using namespace std;
using namespace boost::multi_index;

// Declaración anticipada de DataPaciente 
class DataPaciente;
// Estructura para Boost Multi-Index
struct PacienteData {
    string patientID;          // Identificador unico del paciente
    string patientName;        // Nombre completo del paciente  
    string studyDate;          // Fecha del estudio medico
    string modality;           // Modalidad del estudio (ej: CT, MRI, XRAY)
    string sex;                // Genero del paciente
    long long tamanoArchivo;   // Tamaño del archivo en bytes
    
    // Constructor por defecto necesario para multi_index
    // Inicializa tamanoArchivo a 0
    PacienteData() : tamanoArchivo(0) {}
    
    // Constructor para conversion desde DataPaciente
    // Permite crear PacienteData a partir de otro tipo de estructura
    PacienteData(const DataPaciente& dp);
    
    // Conversion a DataPaciente
    // Convierte esta estructura a otro tipo de dato
    DataPaciente toDataPaciente() const;
};

// Definicion del contenedor multi-index
// Crea un contenedor que permite multiples formas de acceder a los datos
typedef multi_index_container<
    PacienteData,  // Tipo de dato almacenado
    indexed_by<    // Definicion de los indices disponibles
        // Indice primario por ID de paciente (unico y ordenado)
        ordered_unique<member<PacienteData, string, &PacienteData::patientID>>,
        
        // Indice secundario por nombre (puede haber duplicados, ordenado)
        ordered_non_unique<member<PacienteData, string, &PacienteData::patientName>>,
        
        // Indice por modalidad (ordenado, permite multiples valores iguales)
        ordered_non_unique<member<PacienteData, string, &PacienteData::modality>>,
        
        // Indice por genero (ordenado, permite multiples valores iguales)  
        ordered_non_unique<member<PacienteData, string, &PacienteData::sex>>,
        
        // Indice secuencial (mantiene el orden de insercion)
        sequenced<>
    >
> PacienteContainer;  // Tipo definido para el contenedor de pacientes

// Funcion para convertir a minusculas
// Toma una cadena de texto y devuelve una version en minusculas
string aMinusculas(const string& str) {
    // Crea una copia de la cadena original para no modificar la entrada
    string resultado = str;
    
    // Aplica la transformacion a toda la cadena
    // transform recorre cada caracter y aplica la funcion tolower
    // ::tolower es la funcion de la biblioteca estandar C que convierte a minuscula
    transform(resultado.begin(), resultado.end(), resultado.begin(), ::tolower);
    
    // Devuelve la cadena convertida a minusculas
    return resultado;
}

// Funcion para convertir fecha simulada
// Convierte fecha en formato AAAAMMDD a formato DD/MM/AAAA
string convertirFechaSimulada(const string& fecha) {
    // Verifica que la cadena tenga al menos 8 caracteres (AAAAMMDD)
    if (fecha.length() < 8) return "Fecha invalida";
    
    try {
        // Extrae componentes de la fecha
        string anio = fecha.substr(0, 4);  // Primeros 4 caracteres: año
        string mes = fecha.substr(4, 2);   // Siguientes 2 caracteres: mes  
        string dia = fecha.substr(6, 2);   // Siguientes 2 caracteres: dia
        
        // Convierte mes y dia a numeros para validacion
        int iMes = stoi(mes);  // string to int para el mes
        int iDia = stoi(dia);  // string to int para el dia
        
        // Validacion basica de rangos de fecha
        // Nota: esta validacion es simplificada (no considera meses con 30 dias o febrero)
        if (iMes < 1 || iMes > 12 || iDia < 1 || iDia > 31) {
            return "Fecha invalida";
        }
        
        // Construye fecha en nuevo formato: DD/MM/AAAA
        return dia + "/" + mes + "/" + anio;
        
    } catch (...) {
        // Captura cualquier excepcion (stoi falla, etc.)
        return "Fecha invalida";
    }
}


// Clase DataPaciente (definicion completa)
// Gestiona la informacion de pacientes medicos y sus estudios
class DataPaciente {
    private:
        // Datos privados del paciente
        string patientID;               // Identificador unico del paciente
        string patientName;             // Nombre completo del paciente
        string studyDate;               // Fecha del estudio en formato AAAAMMDD
        string studyDateFormateada;     // Fecha formateada para mostrar
        string modality;                // Tipo de estudio (CT, MRI, XRAY, etc.)
        string sex;                     // Genero del paciente
        long long tamanoArchivo;        // Tamano del archivo en bytes

    public:
        // Constructor por defecto - inicializa tamanoArchivo a 0
        DataPaciente() : tamanoArchivo(0) {}
        
        // Carga datos desde un formato de texto compacto separado por |
        // Formato esperado: ID|Nombre|Fecha|Modalidad|Sexo|Tamano
        bool cargarDesdeFormatoCompacto(const string& linea) {
            vector<string> campos;
            string campo;
            size_t inicio = 0;
            size_t fin = linea.find('|');
            
            // Divide la linea usando | como separador
            while (fin != string::npos) {
                campo = linea.substr(inicio, fin - inicio);
                campos.push_back(campo);
                inicio = fin + 1;
                fin = linea.find('|', inicio);
            }
            campo = linea.substr(inicio);
            campos.push_back(campo);
            
            // Verifica que tenga todos los campos necesarios
            if (campos.size() < 6) {
                cerr << "Error: Formato invalido. Se esperaban 6 campos, se encontraron " << campos.size() << endl;
                return false;
            }
            
            // Asigna los valores a los atributos
            patientID = campos[0];
            patientName = campos[1];
            studyDate = campos[2];
            studyDateFormateada = convertirFechaSimulada(studyDate);
            modality = campos[3];
            
            // Convierte codigo de sexo a texto legible
            string sexoCode = campos[4];
            if (sexoCode == "M") sex = "Masculino";
            else if (sexoCode == "F") sex = "Femenino";
            else if (sexoCode == "O") sex = "Otro";
            else sex = sexoCode;
            
            // Convierte tamano del archivo, genera valor por defecto si falla
            try {
                tamanoArchivo = stoll(campos[5]);
            } catch (...) {
                tamanoArchivo = generarTamanoPorModalidad(modality);
            }
            
            return true;
        }
        
        // Genera un tamano de archivo realistico basado en la modalidad del estudio
        long long generarTamanoPorModalidad(const string& modalidad) {
            if (modalidad == "CT") return 50000000LL + (rand() % 100000000LL);      // 50-150 MB
            if (modalidad == "MRI") return 100000000LL + (rand() % 200000000LL);    // 100-300 MB
            if (modalidad == "XRAY") return 10000000LL + (rand() % 40000000LL);     // 10-50 MB
            if (modalidad == "US") return 20000000LL + (rand() % 30000000LL);       // 20-50 MB
            if (modalidad == "PET") return 80000000LL + (rand() % 120000000LL);     // 80-200 MB
            return 50000000LL + (rand() % 50000000LL);                              // 50-100 MB por defecto
        }
        
        // Metodos getter para acceso a los atributos privados
        string getPatientID() const { return patientID; }
        string getPatientName() const { return patientName; }
        string getStudyDate() const { return studyDate; }   
        string getStudyDateFormateada() const { return studyDateFormateada; }
        string getModality() const { return modality; }
        string getSex() const { return sex; }
        long long getSize() const { return tamanoArchivo; }
        
        // Metodos setter para modificar los atributos
        void setPatientID(const string& id) { patientID = id; }
        void setPatientName(const string& name) { patientName = name; }
        void setStudyDate(const string& date) { 
            studyDate = date; 
            studyDateFormateada = convertirFechaSimulada(date);  // Actualiza formato automaticamente
        }
        void setModality(const string& mod) { modality = mod; }
        void setSex(const string& s) { sex = s; }
        void setSize(long long size) { tamanoArchivo = size; }
        
        // Muestra la informacion del paciente en formato legible
        void mostrarInfo() const {
            cout << "Paciente: " << patientName << endl;
            cout << "ID: " << patientID << endl;
            cout << "Fecha del estudio: " << studyDateFormateada << endl;
            cout << "Modalidad: " << modality << endl;
            cout << "Sexo: " << sex << endl;
            long long size = getSize();
            cout << "Tamano archivo: " << size << " bytes (" << (size / 1024.0 / 1024.0) << " MB)" << endl;
            cout << "------------------------------------------------------ " << endl;
        }
        
        // Verifica si un archivo existe en el sistema
        static bool archivoExiste(const string& nombreArchivo) {
            struct stat buffer;
            return (stat(nombreArchivo.c_str(), &buffer) == 0);
        }
};


// Constructor de conversion desde DataPaciente
// Permite crear un PacienteData a partir de un DataPaciente existente
PacienteData::PacienteData(const DataPaciente& dp) {
    // Copia todos los campos desde DataPaciente usando los metodos getter
    patientID = dp.getPatientID();
    patientName = dp.getPatientName();
    studyDate = dp.getStudyDate();
    modality = dp.getModality();
    sex = dp.getSex();
    tamanoArchivo = dp.getSize();
}

// Metodo de conversion a DataPaciente
// Convierte un PacienteData de vuelta a un objeto DataPaciente
DataPaciente PacienteData::toDataPaciente() const {
    DataPaciente dp;
    // Establece todos los campos en el objeto DataPaciente usando los metodos setter
    dp.setPatientID(patientID);
    dp.setPatientName(patientName);
    dp.setStudyDate(studyDate);    
    dp.setModality(modality);
    dp.setSex(sex);
    dp.setSize(tamanoArchivo);
    return dp;
}


// Clase LevelDBManager
// Gestiona la base de datos LevelDB para almacenamiento persistente de pacientes
class LevelDBManager {
    private:
        leveldb::DB* db;        // Puntero a la base de datos LevelDB
        bool connected;         // Estado de conexion a la base de datos
        string dbPath;          // Ruta donde se almacena la base de datos
        
    public:
        // Constructor - inicializa la conexion con LevelDB
        LevelDBManager(const string& path = "./leveldb_data") : db(nullptr), connected(false), dbPath(path) {
            leveldb::Options options;
            options.create_if_missing = true;  // Crea la DB si no existe
            
            // Crea el directorio si no existe
            mkdir(dbPath.c_str(), 0755);
            
            // Intenta abrir la base de datos
            leveldb::Status status = leveldb::DB::Open(options, dbPath, &db);
            
            if (!status.ok()) {
                cerr << "Error abriendo LevelDB: " << status.ToString() << endl;
                connected = false;
                return;
            }
            
            connected = true;
            cout << "LevelDB inicializado correctamente. Base de datos en: " << dbPath << endl;
        }
        
        // Destructor - libera los recursos de la base de datos
        ~LevelDBManager() {
            if (db) {
                delete db;
            }
        }
        
        // Verifica si la conexion a la base de datos esta activa
        bool isConnected() const {
            return connected;
        }
        
        // Cuenta el numero total de pacientes en la base de datos
        long contarPacientes() const {
            if (!connected) return 0;
            
            long count = 0;
            leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
            
            // Recorre todas las entradas contandolas
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                count++;
            }
            
            delete it;
            return count;
        }
        
        // Guarda un nuevo paciente en la base de datos
        // Formato: clave=ID, valor=nombre|fecha|modalidad|sexo|tamano
        bool guardarPaciente(const string& id, const string& nombre, const string& fecha, 
                            const string& modalidad, const string& sexo, long long tamano) {
            if (!connected) return false;
            
            string pacienteData = nombre + "|" + fecha + "|" + modalidad + "|" + sexo + "|" + to_string(tamano);
            leveldb::Status status = db->Put(leveldb::WriteOptions(), id, pacienteData);
            
            if (!status.ok()) {
                cerr << "Error guardando dato: " << status.ToString() << endl;
                return false;
            }
            
            cout << "Paciente guardado en LevelDB: " << id << endl;
            return true;
        }
        
        // Busca un paciente por su ID (clave primaria)
        string buscarPacientePorID(const string& id) {
            if (!connected) return "";
            
            string value;
            leveldb::Status status = db->Get(leveldb::ReadOptions(), id, &value);
            
            if (!status.ok()) {
                if (!status.IsNotFound()) {
                    cerr << "Error buscando dato: " << status.ToString() << endl;
                }
                return "";
            }
            
            return value;
        }
        
        // Obtiene todos los pacientes de la base de datos
        vector<string> buscarTodosLosPacientes() {
            vector<string> resultados;
            if (!connected) return resultados;
            
            leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
            
            // Recorre todas las entradas y las formatea para mostrar
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                string id = it->key().ToString();
                string pacienteData = it->value().ToString();
                
                // Parsea los campos separados por |
                vector<string> campos;
                size_t inicio = 0;
                size_t fin = pacienteData.find('|');
                
                while (fin != string::npos) {
                    campos.push_back(pacienteData.substr(inicio, fin - inicio));
                    inicio = fin + 1;
                    fin = pacienteData.find('|', inicio);
                }
                campos.push_back(pacienteData.substr(inicio));
                
                // Construye string formateado para mostrar
                if (campos.size() >= 5) {
                    string resultado = "ID: " + id +
                                    " | Nombre: " + campos[0] +
                                    " | Fecha: " + campos[1] +
                                    " | Modalidad: " + campos[2] +
                                    " | Sexo: " + campos[3] +
                                    " | Tamano: " + campos[4] + " bytes";
                    resultados.push_back(resultado);
                }
            }
            
            delete it;
            return resultados;
        }
        
        // Busca pacientes por cualquier campo (busqueda parcial case-insensitive)
        // campoIndex: 0=nombre, 1=fecha, 2=modalidad, 3=sexo, 4=tamano
        vector<string> buscarPacientesPorCampo(int campoIndex, const string& valor) {
            vector<string> resultados;
            if (!connected) return resultados;
            
            leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
            string valorBusqueda = aMinusculas(valor);  // Normaliza para busqueda case-insensitive
            
            // Recorre todas las entradas buscando coincidencias
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                string id = it->key().ToString();
                string pacienteData = it->value().ToString();
                
                // Parsea los campos
                vector<string> campos;
                size_t inicio = 0;
                size_t fin = pacienteData.find('|');
                
                while (fin != string::npos) {
                    campos.push_back(pacienteData.substr(inicio, fin - inicio));
                    inicio = fin + 1;
                    fin = pacienteData.find('|', inicio);
                }
                campos.push_back(pacienteData.substr(inicio));
                
                // Verifica si el campo especificado contiene el valor buscado
                if ( (int)campos.size() > campoIndex) {
                    string campoValor = campos[campoIndex];
                    if (aMinusculas(campoValor).find(valorBusqueda) != string::npos) {
                        string resultado = "ID: " + id +
                                        " | Nombre: " + campos[0] +
                                        " | Fecha: " + campos[1] +
                                        " | Modalidad: " + campos[2] +
                                        " | Sexo: " + campos[3] +
                                        " | Tamano: " + campos[4] + " bytes";
                        resultados.push_back(resultado);
                    }
                }
            }
            
            delete it;
            return resultados;
        }
        
        // Elimina un paciente por su ID
        bool eliminarPaciente(const string& id) {
            if (!connected) return false;
            
            leveldb::Status status = db->Delete(leveldb::WriteOptions(), id);
            
            if (!status.ok()) {
                if (!status.IsNotFound()) {
                    cerr << "Error eliminando dato: " << status.ToString() << endl;
                }
                return false;
            }
            
            cout << "Paciente eliminado de LevelDB: " << id << endl;
            return true;
        }
        
        // Elimina todos los pacientes de la base de datos
        void eliminarTodos() {
            if (!connected) return;
            
            leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
            leveldb::WriteOptions write_options;
            
            // Recorre y elimina todas las entradas
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                leveldb::Status status = db->Delete(write_options, it->key());
                if (!status.ok()) {
                    cerr << "Error eliminando clave: " << status.ToString() << endl;
                }
            }
            
            delete it;
            cout << "Eliminacion completada de LevelDB" << endl;
        }
};


// Clase SistemaPacientes
// Clase principal que integra Boost Multi-Index en memoria con LevelDB persistente
class SistemaPacientes {
    private:
        PacienteContainer pacientesContainer;  // Contenedor en memoria con multiples indices
        LevelDBManager leveldb;                // Gestor de base de datos persistente
        
        // Funcion auxiliar para convertir a minusculas (case-insensitive)
        string aMinusculas(const string& str) const {
            string result = str;
            transform(result.begin(), result.end(), result.begin(), ::tolower);
            return result;
        }
            
    public:
        // Constructor - inicializa LevelDB y carga datos existentes
        SistemaPacientes() : leveldb() {
            if (!leveldb.isConnected()) {
                cerr << "Advertencia: No se pudo inicializar LevelDB. Los datos no se persistiran." << endl;
            } else {
                cargarDesdeBaseDeDatos();
            }
        }
        
        // Metodos necesarios para el MenuPrincipal
        
        // Verifica si la base de datos esta conectada
        bool isDBConnected() const {
            return leveldb.isConnected();
        }
        
        // Obtiene la cantidad de pacientes en LevelDB
        long getCantidadLevelDB() const {
            return leveldb.isConnected() ? leveldb.contarPacientes() : 0;
        }
        
        // Carga pacientes desde un archivo de texto con formato compacto
        bool cargarDesdeArchivoCompacto(const string& nombreArchivo) {
            if (!DataPaciente::archivoExiste(nombreArchivo)) {
                cerr << "Error: El archivo '" << nombreArchivo << "' no existe" << endl;
                return false;
            }
            
            ifstream archivo(nombreArchivo);
            if (!archivo.is_open()) {
                cerr << "Error al abrir archivo: " << nombreArchivo << endl;
                return false;
            }
            
            string linea;
            int pacientesCargados = 0;
            int lineasProcesadas = 0;
            
            // Procesa cada linea del archivo
            while (getline(archivo, linea)) {
                lineasProcesadas++;
                if (linea.empty() || linea[0] == '#') continue;  // Salta lineas vacias o comentarios
                
                DataPaciente paciente;
                if (paciente.cargarDesdeFormatoCompacto(linea)) {
                    // Verifica que no exista duplicado antes de agregar
                    if (!existePaciente(paciente.getPatientID())) {
                        agregarPaciente(paciente);
                        pacientesCargados++;
                    } else {
                        cout << "Paciente con ID " << paciente.getPatientID() << " ya existe, omitiendo." << endl;
                    }
                } else {
                    cerr << "Error procesando linea " << lineasProcesadas << ": " << linea << endl;
                }
            }
            
            archivo.close();
            cout << "Cargados " << pacientesCargados << " pacientes desde archivo compacto: " << nombreArchivo << endl;
            return pacientesCargados > 0;
        }
        
        // Verifica si un paciente existe por su ID
        bool existePaciente(const string& id) const {
            auto& index = pacientesContainer.get<0>();  // Indice por ID
            return index.find(id) != index.end();
        }
        
        // Agrega un nuevo paciente al sistema (memoria y persistencia)
        void agregarPaciente(const DataPaciente& paciente) {
            if (existePaciente(paciente.getPatientID())) {
                cout << "El paciente con ID " << paciente.getPatientID() << " ya existe." << endl;
                return;
            }
            
            // Inserta en el contenedor en memoria
            pacientesContainer.insert(PacienteData(paciente));
            
            // Persiste en LevelDB si esta conectado
            if (leveldb.isConnected()) {
                leveldb.guardarPaciente(
                    paciente.getPatientID(),
                    paciente.getPatientName(),
                    paciente.getStudyDate(),
                    paciente.getModality(),
                    paciente.getSex(),
                    paciente.getSize()
                );
            }
        }
        
        // Busca pacientes por nombre (busqueda parcial case-insensitive)
        vector<DataPaciente> buscarPorNombre(const string& nombre) const {
            vector<DataPaciente> resultados;
            if (pacientesContainer.empty()) return resultados;
            
            string nombreBusqueda = aMinusculas(nombre);
            auto& index = pacientesContainer.get<1>();  // Indice por nombre
            
            // Recorre todos los pacientes buscando coincidencias parciales
            for (auto it = index.begin(); it != index.end(); ++it) {
                string nombrePaciente = aMinusculas(it->patientName);
                if (nombrePaciente.find(nombreBusqueda) != string::npos) {
                    resultados.push_back(it->toDataPaciente());
                }
            }
            return resultados;
        }
        
        // Busca pacientes por ID (busqueda exacta)
        vector<DataPaciente> buscarPorID(const string& id) const {
            vector<DataPaciente> resultados;
            if (pacientesContainer.empty()) return resultados;
            
            string idBusqueda = aMinusculas(id);
            auto& index = pacientesContainer.get<0>();  // Indice por ID
            
            auto it = index.find(id);
            if (it != index.end()) {
                string idPaciente = aMinusculas(it->patientID);
                if (idPaciente.find(idBusqueda) != string::npos) {
                    resultados.push_back(it->toDataPaciente());
                }
            }
            return resultados;
        }
        
        // Busca pacientes por modalidad de estudio
        vector<DataPaciente> buscarPorModalidad(const string& modalidad) const {
            vector<DataPaciente> resultados;
            if (pacientesContainer.empty()) return resultados;
            
            string modalidadBusqueda = aMinusculas(modalidad);
            auto& index = pacientesContainer.get<2>();  // Indice por modalidad
            
            // Usa equal_range para obtener todos los pacientes con esa modalidad
            auto range = index.equal_range(modalidad);
            
            for (auto it = range.first; it != range.second; ++it) {
                string modalidadPaciente = aMinusculas(it->modality);
                if (modalidadPaciente.find(modalidadBusqueda) != string::npos) {
                    resultados.push_back(it->toDataPaciente());
                }
            }
            return resultados;
        }
        
        // Busca pacientes por sexo
        vector<DataPaciente> buscarPorSexo(const string& sexo) const {
            vector<DataPaciente> resultados;
            if (pacientesContainer.empty()) return resultados;
            
            string sexoBusqueda = aMinusculas(sexo);
            auto& index = pacientesContainer.get<3>();  // Indice por sexo
            
            auto range = index.equal_range(sexo);
            
            for (auto it = range.first; it != range.second; ++it) {
                string sexoPaciente = aMinusculas(it->sex);
                if (sexoPaciente.find(sexoBusqueda) != string::npos) {
                    resultados.push_back(it->toDataPaciente());
                }
            }
            return resultados;
        }
        
        // Busqueda exacta por ID (devuelve puntero para modificaciones)
        DataPaciente* buscarExactoPorID(const string& id) {
            auto& index = pacientesContainer.get<0>();
            auto it = index.find(id);
            if (it != index.end()) {
                DataPaciente* resultado = new DataPaciente(it->toDataPaciente());
                return resultado;
            }
            return nullptr;
        }
        
        // Busqueda directa en LevelDB (para verificacion de persistencia)
        void buscarEnLevelDB(const string& campo, const string& valor) {
            if (!leveldb.isConnected()) {
                cout << "Error: No hay conexion con LevelDB" << endl;
                return;
            }
            
            int campoIndex = -1;
            if (campo == "nombre") campoIndex = 0;
            else if (campo == "_id") {
                // Busqueda directa por ID (clave primaria)
                string resultado = leveldb.buscarPacientePorID(valor);
                cout << "\n=== RESULTADO DE BUSQUEDA EN LEVELDB ===" << endl;
                if (!resultado.empty()) {
                    vector<string> campos;
                    size_t inicio = 0;
                    size_t fin = resultado.find('|');
                    
                    while (fin != string::npos) {
                        campos.push_back(resultado.substr(inicio, fin - inicio));
                        inicio = fin + 1;
                        fin = resultado.find('|', inicio);
                    }
                    campos.push_back(resultado.substr(inicio));
                    
                    if (campos.size() >= 5) {
                        cout << "ID: " << valor << endl;
                        cout << "Nombre: " << campos[0] << endl;
                        cout << "Fecha: " << campos[1] << endl;
                        cout << "Modalidad: " << campos[2] << endl;
                        cout << "Sexo: " << campos[3] << endl;
                        cout << "Tamano: " << campos[4] << " bytes" << endl;
                    }
                } else {
                    cout << "No se encontro el paciente con ID: " << valor << endl;
                }
                return;
            }
            else if (campo == "modalidad") campoIndex = 2;
            else if (campo == "sexo") campoIndex = 3;
            
            if (campoIndex == -1) {
                cout << "Campo de busqueda no valido" << endl;
                return;
            }
            
            // Busqueda por campos secundarios (requiere escaneo completo)
            auto resultados = leveldb.buscarPacientesPorCampo(campoIndex, valor);
            cout << "\n=== RESULTADOS DE BUSQUEDA EN LEVELDB ===" << endl;
            for (const auto& resultado : resultados) {
                cout << resultado << endl;
                cout << "-------------------" << endl;
            }
            if (resultados.empty()) {
                cout << "No se encontraron resultados en LevelDB" << endl;
            }
        }
        
        // Muestra todos los pacientes en memoria
        void mostrarTodos() const {
            if (pacientesContainer.empty()) {
                cout << "No hay pacientes registrados." << endl;
                return;
            }
            
            auto& index = pacientesContainer.get<4>();  // Indice secuencial (orden de insercion)
            int contador = 1;
            
            for (const auto& paciente : index) {
                cout << "------------------------------------------------------" << endl;
                cout << "           Paciente " << contador++ << endl;
                cout << "------------------------------------------------------" << endl;
                paciente.toDataPaciente().mostrarInfo();
            }
        }
        
        // Verifica sincronizacion entre memoria y base de datos
        void sincronizarConBaseDeDatos() {
            if (!leveldb.isConnected()) {
                cout << "No hay conexion a la base de datos para sincronizar." << endl;
                return;
            }
            
            long pacientesEnBD = leveldb.contarPacientes();
            size_t pacientesEnMemoria = pacientesContainer.size();
            
            cout << "Sincronizacion:" << endl;
            cout << "- Pacientes en memoria (Boost Multi-Index): " << pacientesEnMemoria << endl;
            cout << "- Pacientes en LevelDB: " << pacientesEnBD << endl;
            
            if (pacientesEnBD > (long) pacientesEnMemoria ) {
                cout << "Nota: LevelDB tiene mas pacientes. Los datos estan persistidos correctamente." << endl;
            } else if (pacientesEnBD < (long) pacientesEnMemoria) {
                cout << "Advertencia: Hay pacientes en memoria que no estan en LevelDB." << endl;
            } else {
                cout << "Los datos estan sincronizados." << endl;
            }
        }
        
        // Obtiene cantidad de pacientes en memoria
        size_t getCantidadPacientes() const {
            return pacientesContainer.size();
        }
        
        // Calcula el peso total de archivos en memoria
        long long pesoEnMemoria() const {
            long long peso = 0;
            for (const auto& paciente : pacientesContainer) {
                peso += paciente.tamanoArchivo;
            }
            return peso;
        }
        
        // Elimina un paciente por ID
        bool borrarPaciente(const string& id) {
            auto& index = pacientesContainer.get<0>();
            auto it = index.find(id);
            if (it != index.end()) {
                index.erase(it);
                
                if (leveldb.isConnected()) {
                    leveldb.eliminarPaciente(id);
                }
                return true;
            }
            return false;
        }
        
        // Metodo de compatibilidad para borrado por indice secuencial
        bool borrarPaciente(size_t indice) {
            auto& index = pacientesContainer.get<4>();
            if (indice < index.size()) {
                auto it = index.begin();
                advance(it, indice);
                string id = it->patientID;
                index.erase(it);
                
                if (leveldb.isConnected()) {
                    leveldb.eliminarPaciente(id);
                }
                return true;
            }
            return false;
        }
        
        // Elimina todos los pacientes del sistema
        void borrarTodos() {
            pacientesContainer.clear();
            if (leveldb.isConnected()) {
                leveldb.eliminarTodos();
            }
            cout << "Todos los registros han sido eliminados." << endl;
        }
        
    private:
        // Carga inicial desde base de datos 
        void cargarDesdeBaseDeDatos() {
            if (!leveldb.isConnected()) return;
            cout << "LevelDB conectado. " << leveldb.contarPacientes() << " pacientes en la base de datos." << endl;
        }
};


// Clase MenuPrincipal 
// Gestiona la interfaz de usuario y coordina las operaciones del sistema
class MenuPrincipal {
    private:
        SistemaPacientes sistema;  // Instancia del sistema de pacientes
        
        // Muestra resultados de busqueda de forma formateada
        void mostrarResultadosBusqueda(const vector<DataPaciente>& resultados) const {
            if (resultados.empty()) {
                cout << "No se encontraron resultados." << endl;
            } else {
                cout << "-----------------------------------------------------------------------" << endl;
                cout << "   RESULTADOS DE BUSQUEDA (" << resultados.size() << " encontrados)  " << endl;
                for (size_t i = 0; i < resultados.size(); ++i) {
                    cout << "-----------------------------------------------------------------------" << endl;
                    cout << "        Resultado " << i + 1 << endl;
                    resultados[i].mostrarInfo();
                }
                cout << "-----------------------------------------------------------------------" << endl;
            }
        }
    
public:
    // Metodo principal que ejecuta el menu en bucle
    void ejecutar() {
        int opcion;
        srand(time(nullptr));  // Inicializa semilla para numeros aleatorios
        
        // Muestra advertencia si LevelDB no esta conectado
        if (!sistema.isDBConnected()) {
            cout << "-----------------------------------------------------------------------" << endl;
            cout << "                             ADVERTENCIA" << endl;
            cout << " No se pudo inicializar LevelDB." << endl;
            cout << " Asegurate de tener permisos de escritura en el directorio actual." << endl;
            cout << " Los datos no se guardaran persistentemente." << endl;
            cout << "-----------------------------------------------------------------------" << endl;
            cout << "Presione Enter para continuar..." << endl;
            cin.get();
        }
        
        // Bucle principal del menu
        do {
            mostrarMenu();
            cout << "Seleccione una opcion: ";
            
            // Manejo robusto de entrada incorrecta
            if (!(cin >> opcion)) {
                cin.clear();
                cin.ignore(10000, '\n');
                cout << "Entrada no valida. Por favor ingrese un numero." << endl;
                continue;
            }
            
            cin.ignore();  // Limpia el buffer de entrada
            
            // Ejecuta la opcion seleccionada
            switch(opcion) {
                case 1: cargarArchivoCompacto(); break;
                case 2: sistema.mostrarTodos(); break;
                case 3: subMenuBusqueda(); break;
                case 4: subMenuBorrado(); break;
                case 5: 
                    if (sistema.isDBConnected()) {
                        subMenuLevelDB();
                    } else {
                        cout << "Error: No hay conexion con Base de datos" << endl;
                        cout << "\nPresione Enter para continuar...";
                        cin.get();
                    }
                    break;
                case 6: 
                    if (sistema.isDBConnected()) {
                        sistema.sincronizarConBaseDeDatos();
                    } else {
                        cout << "Error: No hay conexion con Base de datos" << endl;
                    }
                    break;
                case 7: cout << "Saliendo del programa..." << endl; break;
                default: cout << "Opcion no valida. Intente nuevamente." << endl; break;
            }
            
            // Pausa antes de continuar (excepto al salir)
            if (opcion != 7) {
                cout << "\nPresione Enter para continuar...";
                cin.get();
            }
            
        } while (opcion != 7);
    }
    
private:
    // Muestra el menu principal con estadisticas del sistema
    void mostrarMenu() const {
        // Limpia la pantalla (compatible Windows/Linux)
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
        
        cout << "\n---------------------------------------------------------------------------------" << endl;
        cout << "                   SISTEMA DE PACIENTES" << endl;
        cout << "---------------------------------------------------------------------------------" << endl;
        cout << " 1. Cargar archivo de pacientes" << endl;
        cout << " 2. Mostrar todos los pacientes registrados" << endl;
        cout << " 3. Buscar pacientes (Boost Multi-Index)" << endl;
        cout << " 4. Borrar registro" << endl;
        cout << " 5. Buscar en LevelDB" << endl;
        cout << " 6. Sincronizar con LevelDB" << endl;
        cout << " 7. Salir" << endl;
        cout << "---------------------------------------------------------------------------------" << endl;
        // Muestra estadisticas en tiempo real
        cout << " Pacientes en memoria: " << sistema.getCantidadPacientes() << endl;
        cout << " Pacientes en Base de datos: " << sistema.getCantidadLevelDB() << endl;
        cout << " Espacio en memoria utilizado: " << (sistema.pesoEnMemoria() / 1024.0 / 1024.0) << " MB" << endl;
        if (!sistema.isDBConnected()) {
            cout << " Estado BD: DESCONECTADO" << endl;
        } else {
            cout << " Estado BD: CONECTADO" << endl;
        }
        cout << "---------------------------------------------------------------------------------" << endl;
    }
    
    // Permite al usuario cargar un archivo personalizado
    void cargarArchivoCompacto() {
        string nombreArchivo;
        cout << "Ingrese el nombre del archivo(ej: pacientes.txt): ";
        getline(cin, nombreArchivo);
        
        // Limpia espacios en blanco
        if (!nombreArchivo.empty()) {
            nombreArchivo.erase(nombreArchivo.find_last_not_of(" \n\r\t") + 1);
        }
        
        if (nombreArchivo.empty()) {
            cout << "Nombre de archivo no valido." << endl;
            return;
        }
        
        if (sistema.cargarDesdeArchivoCompacto(nombreArchivo)) {
            cout << "Archivo cargado exitosamente." << endl;
        } else {
            cout << "Error al cargar el archivo compacto." << endl;
        }
    }

    // Submenu para busquedas usando Boost Multi-Index
    void subMenuBusqueda() {
        int opcion;
        do {
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif

            cout << "-------------------------------------------------------" << endl;
            cout << "\n           BUSQUEDA CON BOOST MULTI-INDEX           " << endl;
            cout << "-------------------------------------------------------" << endl;
            cout << " 1. Buscar por nombre " << endl;
            cout << " 2. Buscar por ID" << endl;
            cout << " 3. Buscar por modalidad " << endl;
            cout << " 4. Buscar por sexo" << endl;
            cout << " 5. Busqueda exacta por ID" << endl;
            cout << " 6. Volver al menu principal" << endl;
            cout << "-------------------------------------------------------" << endl;
            cout << " Seleccione una opcion: ";
            
            if (!(cin >> opcion)) {
                cin.clear();
                cin.ignore(10000, '\n');
                cout << "Entrada no valida." << endl;
                continue;
            }
            
            cin.ignore();
            
            if (opcion >= 1 && opcion <= 5) {
                string termino;
                vector<DataPaciente> resultados;
                
                switch(opcion) {
                    case 1: 
                        cout << "Ingrese el nombre o parte del nombre: ";
                        getline(cin, termino);
                        resultados = sistema.buscarPorNombre(termino); 
                        cout << "[Busqueda en indice ordenado por nombre:]" << endl;
                        break;
                    case 2: 
                        cout << "Ingrese el ID o parte del ID: ";
                        getline(cin, termino);
                        resultados = sistema.buscarPorID(termino); 
                        cout << "[Busqueda en indice unico por ID:]" << endl;
                        break;
                    case 3: 
                        cout << "Ingrese la modalidad (CT, MRI, XRAY, US, PET): ";
                        getline(cin, termino);
                        resultados = sistema.buscarPorModalidad(termino); 
                        cout << "[Busqueda en indice agrupado por modalidad:]" << endl;
                        break;
                    case 4: 
                        cout << "Ingrese el sexo (M, F, Otro): ";
                        getline(cin, termino);
                        resultados = sistema.buscarPorSexo(termino); 
                        cout << "[Busqueda en indice agrupado por sexo:]" << endl;
                        break;
                    case 5:
                        cout << "Ingrese el ID exacto: ";
                        getline(cin, termino);
                        {
                            // Busqueda exacta que devuelve puntero para mayor eficiencia
                            DataPaciente* resultadoExacto = sistema.buscarExactoPorID(termino);
                            if (resultadoExacto != nullptr) {
                                cout << "---------------------------------------------------------------------------------" << endl;
                                cout << "                                RESULTADO EXACTO                                 " << endl;
                                cout << "---------------------------------------------------------------------------------" << endl;
                                resultadoExacto->mostrarInfo();
                                delete resultadoExacto; // Liberar memoria
                            } else {
                                cout << "No se encontro paciente con ID: " << termino << endl;
                            }
                        }
                        cout << "[Busqueda exacta en indice unico:]" << endl;
                        break;
                }
                
                // Muestra resultados para busquedas multiples
                if (opcion >= 1 && opcion <= 4) {
                    mostrarResultadosBusqueda(resultados);
                }
                
                if (!resultados.empty() || opcion == 5) {
                    cout << "\nPresione Enter para continuar...";
                    cin.get();
                }
            }
            
        } while (opcion != 6);
    }
    
    // Submenu para busquedas directas en LevelDB
    void subMenuLevelDB() {
        int opcion;
        do {
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif

            cout << "\n------------------------------------------------------------------" << endl;
            cout << "                 BUSQUEDA EN BASE DE DATOS (Clave-Valor)               " << endl;
            cout << "------------------------------------------------------------------" << endl;
            cout << " 1. Buscar por nombre" << endl;
            cout << " 2. Buscar por ID" << endl;
            cout << " 3. Buscar por modalidad" << endl;
            cout << " 4. Buscar por sexo" << endl;
            cout << " 5. Volver al menu principal" << endl;
            cout << "------------------------------------------------------------------" << endl;
            cout << " Seleccione una opcion: ";
            
            if (!(cin >> opcion)) {
                cin.clear();
                cin.ignore(10000, '\n');
                cout << "Entrada no valida." << endl;
                continue;
            }
            
            cin.ignore();
            
            if (opcion >= 1 && opcion <= 4) {
                string termino;
                string campo;
                
                switch(opcion) {
                    case 1: campo = "nombre"; break;
                    case 2: campo = "_id"; break;  // Busqueda directa por clave
                    case 3: campo = "modalidad"; break;
                    case 4: campo = "sexo"; break;
                }
                
                cout << "Ingrese el termino de busqueda: ";
                getline(cin, termino);
                
                sistema.buscarEnLevelDB(campo, termino);
                
                cout << "\nPresione Enter para continuar...";
                cin.get();
            }
            
        } while (opcion != 5);
    }
    
    // Submenu para operaciones de borrado
    void subMenuBorrado() {
        int opcion;
        do {
            #ifdef _WIN32
                system("cls");
            #else
                system("clear");
            #endif
            
            cout << "----------------------------------------------------" << endl;
            cout << "\n                  MENU DE BORRADO                 " << endl;
            cout << "----------------------------------------------------" << endl;
            cout << " 1. Borrar paciente por ID" << endl;
            cout << " 2. Borrar todos los registros" << endl;
            cout << " 3. Volver al menu principal" << endl;
            cout << "----------------------------------------------------" << endl;
            cout << " Seleccione una opcion: ";
            
            if (!(cin >> opcion)) {
                cin.clear();
                cin.ignore(10000, '\n');
                cout << "Entrada no valida." << endl;
                continue;
            }
            
            cin.ignore();
            
            if (opcion == 1) {
                // Verifica que haya pacientes antes de borrar
                if (sistema.getCantidadPacientes() == 0) {
                    cout << "No hay pacientes registrados para borrar." << endl;
                    cout << "\nPresione Enter para continuar...";
                    cin.get();
                    continue;
                }
                
                sistema.mostrarTodos();
                cout << "Ingrese el ID del paciente a borrar: ";
                string id;
                getline(cin, id);
                
                if (id.empty()) {
                    cout << "ID no valido." << endl;
                } else {
                    // Busca el paciente y pide confirmacion antes de borrar
                    DataPaciente* paciente = sistema.buscarExactoPorID(id);
                    if (paciente != nullptr) {
                        cout << "Paciente encontrado: " << paciente->getPatientName() << endl;
                        cout << "¿Esta seguro de que desea borrar este paciente? (s/n): ";
                        char confirmacion;
                        cin >> confirmacion;
                        cin.ignore();
                        
                        if (confirmacion == 's' || confirmacion == 'S') {
                            if (sistema.borrarPaciente(id)) {
                                cout << "Paciente borrado exitosamente." << endl;
                            } else {
                                cout << "Error al borrar el paciente." << endl;
                            }
                        } else {
                            cout << "Operacion cancelada." << endl;
                        }
                        delete paciente;
                    } else {
                        cout << "No se encontro paciente con ID: " << id << endl;
                    }
                }
                
            } else if (opcion == 2) {
                // Borrado masivo con confirmacion
                cout << "¿Esta seguro de que desea borrar todos los registros? (s/n): ";
                char confirmacion;
                cin >> confirmacion;
                cin.ignore();
                
                if (confirmacion == 's' || confirmacion == 'S') {
                    sistema.borrarTodos();
                } else {
                    cout << "Operacion cancelada." << endl;
                }
            }
            
            if (opcion != 3) {
                cout << "\nPresione Enter para continuar...";
                cin.get();
            }
            
        } while (opcion != 3);
    }
};


// Funcion principal del programa
int main() {

    try {
        // Mensaje de inicio del sistema
        cout << "Inicializando sistema de pacientes..." << endl;
        
        // Crea la instancia del menu principal
        MenuPrincipal menu;
        
        // Ejecuta el bucle principal de la aplicacion
        menu.ejecutar();
        
    } catch (const exception& e) {
        // Captura y muestra cualquier excepcion no controlada
        cerr << "Error fatal: " << e.what() << endl;
        return 1;  
    }

    return 0;
}

