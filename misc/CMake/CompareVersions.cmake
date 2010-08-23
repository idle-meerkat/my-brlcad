# Note that these routines don't (yet) handle letters in versions

MACRO(COMPARE_VERSIONS version1 version2 resultvar)
   MESSAGE("version1: ${version1} version2: ${version2}") 
   STRING(REGEX REPLACE "[.-]" ";" LIST1 "${version1}")
   STRING(REGEX REPLACE "[.-]" ";" LIST2 "${version2}")
   list(LENGTH LIST1 LEN1)
   list(LENGTH LIST2 LEN2)
   MESSAGE("len1: ${LEN1} len2: ${LEN2}")
   WHILE (LEN1 LESS LEN2)
       LIST(APPEND LIST1 "0")
       list(LENGTH LIST1 LEN1)
   ENDWHILE()      
   WHILE (LEN2 LESS LEN1)
       LIST(APPEND LIST2 "0")
       list(LENGTH LIST2 LEN2)
   ENDWHILE()      
   SET(LCOUNT ${LEN2})
   MESSAGE("LCOUNT: ${LCOUNT}")
   SET(COUNT 0)
   while(COUNT LESS LCOUNT)
       LIST(GET LIST1 ${COUNT} NUM1)
       LIST(GET LIST2 ${COUNT} NUM2)
       MESSAGE("NUM1: ${NUM1}")
       MESSAGE("NUM2: ${NUM2}")
       STRING(LENGTH ${NUM1} N1LEN)
       STRING(LENGTH ${NUM2} N2LEN)
       WHILE (N1LEN LESS N2LEN)
	  SET(NUM1 "0${NUM1}")
          STRING(LENGTH ${NUM1} N1LEN)
       ENDWHILE()      
       WHILE (LEN2 GREATER LEN1)
          SET(NUM2 "0${NUM2}")
          STRING(LENGTH ${NUM2} N2LEN)
       ENDWHILE()
       LIST(INSERT LIST1 ${COUNT} ${NUM1})
       LIST(INSERT LIST2 ${COUNT} ${NUM2})
       math(EXPR RMVCNT "${COUNT} + 1")
       LIST(REMOVE_AT LIST1 ${RMVCNT})
       LIST(REMOVE_AT LIST2 ${RMVCNT})
       math(EXPR COUNT "${COUNT} + 1")
    endwhile()
    LIST(REVERSE LIST1)
    LIST(REVERSE LIST2)
    FOREACH(num ${LIST1})
      SET(compnum1 "${num}${compnum1}")
    ENDFOREACH ()
    FOREACH(num ${LIST2})
      SET(compnum2 "${num}${compnum2}")
    ENDFOREACH ()
    MESSAGE("compnum1: ${compnum1} compnum2: ${compnum2}")
    IF(${compnum1} GREATER ${compnum2})
	SET(${resultvar} 1)
    ENDIF()
    IF(${compnum2} GREATER ${compnum1})
	SET(${resultvar} 2)
    ENDIF()
    if (NOT ${resultvar})
	SET(${resultvar} 0)
    endif()
ENDMACRO(COMPARE_VERSIONS)

