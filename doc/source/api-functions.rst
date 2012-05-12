
.. c:function:: int canopen_frame_parse(canopen_frame_t *canopen_frame, struct can_frame *can_frame)
   
    Parse the CAN frame data payload as a CANopen packet.


.. c:function:: int canopen_frame_pack(canopen_frame_t *canopen_frame, struct can_frame *can_frame)
   
    Pack a CANopen frame into a CAN frame that is ready to be transmitted
    to the CAN network.
    
    * *canopen_frame*: a pointer to a CANopen frame data struct (input)
    * *can_frame*:     a pointer to a CAN frame data struct (output)
     

.. c:function:: int canopen_frame_dump_short(canopen_frame_t *frame)
   
    Print a CAN frame in a human-readable (compact) format to the
    standard output.


.. c:function:: int canopen_frame_dump_verbose(canopen_frame_t *frame)
   
    Print a CAN frame in a human-readable (verbose) format to the
    standard output.


.. c:function:: canopen_frame_t *canopen_frame_new()
   
    Allocate a new CANopen frame data struct.


.. c:function:: void canopen_frame_free(canopen_frame_t *frame)
   
    Free the memory associated with the CANopen frame data struct.


.. c:function:: int canopen_frame_set_nmt_mc(canopen_frame_t *frame, uint8_t cs, uint8_t node)
   
    Initiate a Network ManagemenT Module Control frame. 

    Arguments:
        
        frame (canopen_frame_t *): A pointer to a CANopen frame struct
        in which the NMT MC frame with be prepared.
        
        cs (uint8_t): command specifyer, taking one of the following values
          
            * CANOPEN_NMT_MC_CS_START
            * CANOPEN_NMT_MC_CS_STOP
            * CANOPEN_NMT_MC_CS_PREOP
            * CANOPEN_NMT_MC_CS_RESET_APP
            * CANOPEN_NMT_MC_CS_RESET_COM
    
        node (uint8_t): node id for the target slave.
    
    Returns: 
        
        An integer indicating success (0) or failure (<0).
    
