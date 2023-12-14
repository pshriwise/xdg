#ifndef _XDG_ERROR_H
#define _XDG_ERROR_H

// Borrowed from OpenMC
#include <string>

void fatal_error(const std::string& message, int err=-1);

#endif