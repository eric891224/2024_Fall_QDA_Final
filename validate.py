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

def h_mem_circuit(num_qbits: int):
    qc = QuantumCircuit(num_qbits)
    # Apply the h-gates for each qubit
    for qubit in range(num_qbits):
        qc.h(qubit)

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

def cp_mem_circuit(num_qbits: int):
    qc = QuantumCircuit(num_qbits)
    
    for qubit in range(num_qbits):
        qc.rx(math.pi / 4, qubit)
    qc.cp(math.pi/2, 1, 0)

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

def qft_mem_circuit(num_qbits: int):
    qc = QuantumCircuit(num_qbits)
    
    for i in range(num_qbits):
        qc.h(i)
        for j in range(i+1, num_qbits):
            angle = math.pi / (2**(j-i))
            qc.cp(angle, j, i)

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


def rx_mem(num_qbits = 3):
    state = rx_mem_circuit(num_qbits)
    checker(state, './rx.mem', "RX MEM")

def rx_raid(num_qbits = 3):
    state = rx_mem_circuit(num_qbits)
    checker(state, './rx.raid', "RX RAID")

def h_mem(num_qbits = 3):
    state = h_mem_circuit(num_qbits)
    checker(state, './h.mem', "H MEM")

def h_raid(num_qbits = 3):
    state = h_mem_circuit(num_qbits)
    checker(state, './h.raid', "H RAID")

def qft_raid(num_qbits):
    state = qft_mem_circuit(num_qbits)
    checker(state, './qft.raid', "QFT RAID")

def qft_mem(num_qbits):
    state = qft_mem_circuit(num_qbits)
    # checker(state, './qft.mem', "QFT RAID")

def cp_mem(num_qbits = 3):
    state = cp_mem_circuit(num_qbits)
    checker(state, './cp.mem', "CP MEM")

def cp_raid(num_qbits = 3):
    state = cp_mem_circuit(num_qbits)
    checker(state, './cp.raid', "CP RAID")

def main():
    # rx_mem(30)
    # rx_raid(30)
    # h_mem(30)
    # h_raid(30)
    # cp_mem(30)
    # cp_raid(30)
    qft_mem(31)
    # qft_raid(28)

def debug():
    file_state = np.fromfile('./cp.raid', dtype=np.complex128)
    print(file_state)

if __name__ == '__main__':
    main()
    # debug()