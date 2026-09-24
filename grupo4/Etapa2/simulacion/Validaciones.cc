#include <regex>
#include "Validaciones.h"

bool validarOrigenDestino( const std::string & valor ) {
    static const std::regex patron( "^((CLI|INT)_[0-9]{2}|SERV)$" );
    return std::regex_match( valor, patron );
}

bool validarTipoMensaje( const std::string & valor ) {
    static const std::regex patron( "^[0-9]{2}$" );
    return std::regex_match( valor, patron );
}

bool validarCategoria( const std::string & valor ) {
    static const std::regex patron( "^[a-zA-Z]{3,20}$" );
    return std::regex_match( valor, patron );
}

bool validarProducto( const std::string & valor ) {
    static const std::regex patron( "^[a-zA-Z0-9\\-]{1,50}$" );
    return std::regex_match( valor, patron );
}

bool validarPrecio( const std::string & valor ) {
    static const std::regex patron( "^[0-9]{1,6}(\\.[0-9]{1,2})?$" );
    return std::regex_match( valor, patron );
}

bool validarStock( const std::string & valor ) {
    static const std::regex patron( "^[0-9]{1,5}$" );
    return std::regex_match( valor, patron );
}

bool validarCount( const std::string & valor ) {
    static const std::regex patron( "^[0-9]{1,3}$" );
    return std::regex_match( valor, patron );
}