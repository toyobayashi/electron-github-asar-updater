if not exist .\out\win32\ia32 mkdir .\out\win32\ia32
if not exist .\out\win32\x64 mkdir .\out\win32\x64

cmake -A Win32 -S . -B .\out\win32\ia32
cmake -A x64 -S . -B .\out\win32\x64
