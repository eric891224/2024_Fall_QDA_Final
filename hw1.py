import os
import math
import qiskit_aer
import numpy as np
from qiskit import QuantumCircuit

qubits = 3

def validate(sv, file_name):
    file_state = np.fromfile(file_name, dtype=np.complex128)
    # print("[C version]: \n", file_state)
    if (np.abs(file_state[0]-1) < 1e-9): # can not be the init state
        return False
    return np.all(np.abs(sv-file_state) < 1e-9)

def checker(state, file_name, hw_id):
    print(hw_id)
    print(state)
    if os.path.isfile(file_name):
        if validate(state, file_name):
            print("Validation Success")
        else:
            print("Validation Failed")
    print("--------")

def hw1_2_1_circuit():
    qc = QuantumCircuit(qubits)
    for qubit in range(qubits):
        qc.rx(math.pi / 4, qubit)

    qc.save_statevector()
    simulator = qiskit_aer.AerSimulator(
        method='statevector',
        device='CPU',
        precision='double',
        fusion_enable=False,
        blocking_enable=False,
    )
    job = simulator.run(qc, cuStateVec_enable=False)
    return job.result().data()['statevector'].data

def hw1_2_2_circuit():
    qc = QuantumCircuit(qubits)
    # Apply the h-gates for each qubit
    # for qubit in range(qubits):
    #     qc.h(qubit)

    qc.save_statevector()
    simulator = qiskit_aer.AerSimulator(
        method='statevector',
        device='CPU',
        precision='double',
        fusion_enable=False,
        blocking_enable=False,
    )
    job = simulator.run(qc, cuStateVec_enable=False)
    return job.result().data()['statevector'].data

def hw1_2_3_circuit():
    qc = QuantumCircuit(qubits)
    # Apply the h-gates for each qubit
    # for qubit in range(qubits):
    #     qc.h(qubit)
    # # Apply cp-gates for each qubit as the document
    # qc.cp(math.pi/4, 1, 0)
    # qc.cp(math.pi/4, 2, 0)

    qc.save_statevector()
    simulator = qiskit_aer.AerSimulator(
        method='statevector',
        device='CPU',
        precision='double',
        fusion_enable=False,
        blocking_enable=False,
    )
    job = simulator.run(qc, cuStateVec_enable=False)
    return job.result().data()['statevector'].data

def hw1_2_4_circuit():
    qc = QuantumCircuit(qubits)
    # Implement a qft circuit
    # qc.h(0)
    # qc.cp(math.pi/2, 1, 0)
    # qc.cp(math.pi/4, 2, 0)

    # qc.h(1)
    # qc.cp(math.pi/2, 2, 1)

    # qc.h(2)

    qc.save_statevector()
    simulator = qiskit_aer.AerSimulator(
        method='statevector', 
        device='CPU', 
        precision='double', 
        fusion_enable=False,
        blocking_enable=False,
    )
    job = simulator.run(qc, cuStateVec_enable=False)
    return job.result().data()['statevector'].data

def rx_mem_circuit(num_qbits: int):
    qc = QuantumCircuit(num_qbits)
    for qubit in range(num_qbits):
        qc.rx(math.pi / 4, qubit)

    qc.save_statevector()
    simulator = qiskit_aer.AerSimulator(
        method='statevector',
        device='CPU',
        precision='double',
        fusion_enable=False,
        blocking_enable=False,
    )
    job = simulator.run(qc, cuStateVec_enable=False)
    return job.result().data()['statevector'].data

def rx_raid_circuit(num_qbits: int):
    qc = QuantumCircuit(num_qbits)
    for qubit in range(num_qbits):
        qc.rx(math.pi / 4, qubit)

    qc.save_statevector()
    simulator = qiskit_aer.AerSimulator(
        method='statevector',
        device='CPU',
        precision='double',
        fusion_enable=False,
        blocking_enable=False,
    )
    job = simulator.run(qc, cuStateVec_enable=False)
    return job.result().data()['statevector'].data

def hw1_2_1():
    state = hw1_2_1_circuit()
    checker(state, "./rx.txt", "[hw1_3_1]")

def hw1_2_2():
    state = hw1_2_2_circuit()
    checker(state, "./h.txt", "[hw1_3_2]")

def hw1_2_3():
    state = hw1_2_3_circuit()
    checker(state, "./cp.txt", "[hw1_3_3]")

def hw1_2_4():
    state = hw1_2_4_circuit()
    checker(state, "./qft.txt", "[hw1_3_4]")

def rx_mem(num_qbits = 3):
    state = rx_mem_circuit(num_qbits)
    checker(state, './rx.mem', "RX MEM")

def rx_raid(num_qbits = 3):
    state = rx_raid_circuit(num_qbits)
    checker(state, './state', "RX RAID")

def main():
    # hw1_2_1()
    # hw1_2_2()
    # hw1_2_3()
    # hw1_2_4()
    # rx_mem()
    rx_raid(20)

if __name__ == '__main__':
    main()