module servo_controller (
    input  wire clk_50mhz,    // Clock de 50 MHz
    input  wire [1:0] cmd_in, // Entrada de 2 Fios do ESP32
    output wire pwm1,         // Saída 1 (Coca)
    output wire pwm2,         // Saída 2 (Guarana)
    output wire pwm3          // Saída 3 (Sprite)
);
    // Constantes pra tempo e frequencia
    parameter PWM_PERIOD  = 1000000;
    parameter PULSE_1_5MS = 75000;
    parameter PULSE_0_8MS = 40000;
    parameter PULSE_2_5MS = 125000;
    parameter TIME_1_SEC  = 50000000;
    parameter TIME_2_SEC  = 100000000;
    
    // Tempo de espera de 10 milissegundos (500.000 pulsos de clock)
    parameter TIME_SETTLE = 500000; 

    // Registradores
    reg [19:0] pwm_counter = 0;
    reg [26:0] routine_timer = 0;
    reg [19:0] active_pulse = PULSE_1_5MS; 
    reg is_running = 0;
    reg [1:0] latched_cmd = 0; 

    // Registradores pra delay
    reg waiting_cmd = 0;
    reg [19:0] settle_timer = 0;

    // Gerador da freq do pwm (50Hz)
    always @(posedge clk_50mhz) begin
        if (pwm_counter >= PWM_PERIOD - 1) begin
            pwm_counter <= 0;
        end else begin
            pwm_counter <= pwm_counter + 1;
        end
    end

    // Controlador da sequência de movimentos
    always @(posedge clk_50mhz) begin
        
        // Pra estabilizar o sinal
        if (cmd_in != 2'b00 && !is_running && !waiting_cmd) begin
            waiting_cmd <= 1;   // Inicia a contagem de espera
            settle_timer <= 0;
        end

        // Timer antes de ler o esp
        if (waiting_cmd) begin
            settle_timer <= settle_timer + 1;
            if (settle_timer > TIME_SETTLE) begin
                is_running <= 1;
                routine_timer <= 0;
                latched_cmd <= cmd_in; // AGORA SIM! Tira a foto com o sinal estável (11)
                waiting_cmd <= 0;
            end
        end

        // Rotina dos motores
        if (is_running) begin
            routine_timer <= routine_timer + 1;
            
            if (routine_timer < TIME_1_SEC) begin
                active_pulse <= PULSE_0_8MS;  // solta latinha
            end 
            else if (routine_timer < TIME_2_SEC) begin
                active_pulse <= PULSE_2_5MS;  // pega latinha
            end 
            else begin
                is_running <= 0;
                routine_timer <= 0;
                active_pulse <= PULSE_1_5MS;  // volta pro meio
                latched_cmd <= 2'b00;         // limpa a memória
            end
        end else if (!waiting_cmd) begin
            active_pulse <= PULSE_1_5MS;      // fica no meio
        end
    end

    // Demux pras saidas
    wire sig_routine = (pwm_counter < active_pulse); 
    wire sig_center  = (pwm_counter < PULSE_1_5MS);  

    // Roteamento
    assign pwm1 = (latched_cmd == 2'b01) ? sig_routine : sig_center;
    assign pwm2 = (latched_cmd == 2'b10) ? sig_routine : sig_center;
    assign pwm3 = (latched_cmd == 2'b11) ? sig_routine : sig_center;

endmodule