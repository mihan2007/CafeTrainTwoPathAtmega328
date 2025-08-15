// protection.h
#ifdef __cplusplus
extern "C" {
	#endif
	void check_and_send_overload_stop(void);
	void checkLocoMovementTimeout(void);
	void stopLocoDueToTimeout(void);   // <Ч добавь прототип
	#ifdef __cplusplus
}
#endif
