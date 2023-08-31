/*
 *  -----------------------------------------------------------------------
 *  NEC Vector Host MUMPS Library Interface
 *  -----------------------------------------------------------------------
 */

#include <VE_Metis.h>

/*
 *  ---------------------------------------------------------------------------
 *  VE_Metis_setdefaultoptions
 *  ---------------------------------------------------------------------------
 */

int VE_Metis_setdefaultoptions( int * options ) {
  int ret=1;
  int64_t symid;
  vhcall_handle handle;
  vhcall_args *ca;
  uint64_t retval = -1;
  
  char * env_metis_lib_name;
  char metis_lib_name[ NECVH_MAX_LIBNAME ];

  /*
   *  -----------------------------------------------------------------------
   *  Set the library name ( check environment )
   *  -----------------------------------------------------------------------
   */
  
  if ( env_metis_lib_name = getenv( "VH_MUMPS_LIBRARY" ) ) {
    size_t lenv = strlen( env_metis_lib_name );
    if ( strlen( env_metis_lib_name ) > (size_t)NECVH_MAX_LIBNAME ) {
      fprintf( stdout , "%s @ %d failed : metis library name too long (%s)\n" , basename(__FILE__) , __LINE__ , env_metis_lib_name );
      fflush( stdout );
      (void) mumps_abort();
    }
    else {
      memcpy( ( void * ) metis_lib_name , ( void * ) env_metis_lib_name , lenv * sizeof( char) );
      metis_lib_name[ lenv ] = '\0';
      fprintf( stdout , "%s @ %d input library %s\n" , basename(__FILE__) , __LINE__ , metis_lib_name );
      fflush( stdout );
    }
  }  
  else {
    strcpy( metis_lib_name , "libvh.so" );
  }
  
  /*
   *  -----------------------------------------------------------------------
   *  Load VH C library
   *  -----------------------------------------------------------------------
   */
  
  handle = vhcall_install( metis_lib_name );
  if (handle == (vhcall_handle)-1) {
    perror("vhcall_install");
    fprintf( stdout , "%s @ %d failed : can't install library %s\n" , basename(__FILE__) , __LINE__ , metis_lib_name );
    fprintf( stdout , "Please check your LD_LIBRARY_PATH variable\n");
    fflush( stdout );
    (void) mumps_abort();
  }
  
  /*
   *  -----------------------------------------------------------------------
   *  Find VH C library function
   *  -----------------------------------------------------------------------
   */
  
  symid = vhcall_find(handle, "METIS_SetDefaultOptions");
  if ( symid == -1 ) {
    fprintf( stdout , "%s @ %d failed : can't find symbol METIS_SetDefaultOptions\n" , basename(__FILE__) , __LINE__ );
    perror("vhcall_find");
    fflush( stdout );
    (void) mumps_abort();
  }
  
  /*
   *  -----------------------------------------------------------------------
   *  Allocate arguments object for VH library function
   *  -----------------------------------------------------------------------
   */
  
  ca = vhcall_args_alloc();
  if ( ! ca ) {
    perror("vhcall_args_alloc");
    fprintf( stdout , "%s @ %d failed : unable to allocate function's argument list\n" , basename(__FILE__) , __LINE__ );
    fflush( stdout );
    (void) mumps_abort();
  }
  
  /*
   *  -----------------------------------------------------------------------
   *  Set argument
   *  -----------------------------------------------------------------------
   */
  
  ret = vhcall_args_set_pointer( ca , VHCALL_INTENT_INOUT , 0 , options , METIS_NOPTIONS * sizeof( idx_t ) );
  if ( ret ) {
    perror("vhcall_args_set_pointer");
    fprintf( stdout , "%s @ %d failed : unable to set argument.\n" , basename(__FILE__) , __LINE__ );
    fflush( stdout );
    (void) mumps_abort();
  }
  
  /*
   *  -----------------------------------------------------------------------
   *  Invoke VH C library function
   *  -----------------------------------------------------------------------
   */
  
  ret = vhcall_invoke_with_args( symid , ca , &retval );
  if ( ret ) {
    perror("vhcall_invoke_with_args");
    fprintf( stdout , "%s @ %d failed : unable to invoke the symbol from library\n" , basename(__FILE__) , __LINE__ );
    fflush( stdout );
    (void) mumps_abort();
  }

  /*
   *  -----------------------------------------------------------------------
   *  Free args
   *  -----------------------------------------------------------------------
   */
  
  vhcall_args_free( ca );
  
  /*
   *  -----------------------------------------------------------------------
   *  Uninstall the library
   *  -----------------------------------------------------------------------
   */

  if ( vhcall_uninstall( handle ) ) {
    perror("vhcall_uninstall");
    fprintf( stdout , "cvhmetis_nodend failed : unable to uninstall the library\n" , basename(__FILE__) , __LINE__ );
    fflush( stdout );
    (void) mumps_abort();
  }

  return( (int) retval );
}
