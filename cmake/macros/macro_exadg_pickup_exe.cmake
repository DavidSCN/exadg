MACRO(EXADG_PICKUP_EXE FILE_NAME TARGET_NAME EXE_NAME)

    ADD_EXECUTABLE(${TARGET_NAME} ${FILE_NAME})
    DEAL_II_SETUP_TARGET(${TARGET_NAME})
    set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${EXE_NAME})
    TARGET_LINK_LIBRARIES(${TARGET_NAME} exadg)

ENDMACRO(EXADG_PICKUP_EXE)