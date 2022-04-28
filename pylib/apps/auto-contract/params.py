import rbc_ukqcd_params as rup

tag = "trajs"
rup.dict_params["test-4nt8"][tag] = list(range(1000, 1400, 100))
rup.dict_params["test-4nt16"][tag] = list(range(1000, 1400, 100))
rup.dict_params["48I"][tag] = list(range(1000, 3000, 5))
rup.dict_params["24D"][tag] = list(range(1000, 10000, 10))
rup.dict_params["24DH"][tag] = list(range(200, 1000, 10))
rup.dict_params["32Dfine"][tag] = list(range(1000, 10000, 10))
rup.dict_params["16IH2"][tag] = list(range(1000, 10000, 10))
rup.dict_params["32IfineH"][tag] = list(range(1000, 10000, 10))
rup.dict_params["32IcoarseH1"][tag] = list(range(300, 2000, 10))
rup.dict_params["24IH1"][tag] = list(range(1000, 10000, 10))
rup.dict_params["24IH2"][tag] = list(range(1000, 10000, 10))
rup.dict_params["24IH3"][tag] = list(range(1000, 10000, 10))
rup.dict_params["24IH4"][tag] = list(range(1000, 10000, 10))
rup.dict_params["32IH1"][tag] = list(range(1000, 10000, 10))
rup.dict_params["32IH2"][tag] = list(range(1000, 10000, 10)) + list(range(1002, 10000, 10))
rup.dict_params["32IH3"][tag] = list(range(1000, 10000, 10))

tag = "n_points_psel"
rup.dict_params["test-4nt8"][tag] = 6
rup.dict_params["test-4nt16"][tag] = 32
rup.dict_params["48I"][tag] = 2048
rup.dict_params["24D"][tag] = 1024
rup.dict_params["24DH"][tag] = 1024
rup.dict_params["32IfineH"][tag] = 512
rup.dict_params["32IcoarseH1"][tag] = 512
rup.dict_params["16IH2"][tag] = 256
rup.dict_params["24IH3"][tag] = 512
rup.dict_params["24IH2"][tag] = 512
rup.dict_params["24IH1"][tag] = 512
rup.dict_params["32IH1"][tag] = 512
rup.dict_params["32IH2"][tag] = 512
rup.dict_params["32IH3"][tag] = 512

tag = "n_exact_wsrc"
rup.dict_params["test-4nt8"][tag] = 2
rup.dict_params["48I"][tag] = 2

tag = "prob_exact_wsrc"
rup.dict_params["test-4nt16"][tag] = 1/8
rup.dict_params["16IH2"][tag] = 1/16
rup.dict_params["24D"][tag] = 1/32
rup.dict_params["24DH"][tag] = 1/32
rup.dict_params["32IfineH"][tag] = 1/32
rup.dict_params["32IcoarseH1"][tag] = 1/32
rup.dict_params["24IH1"][tag] = 1/32
rup.dict_params["24IH2"][tag] = 1/32
rup.dict_params["24IH3"][tag] = 1/32
rup.dict_params["32IH1"][tag] = 1/32
rup.dict_params["32IH2"][tag] = 1/32
rup.dict_params["32IH3"][tag] = 1/32

tag = "n_per_tslice_smear"
rup.dict_params["test-4nt8"][tag] = 2
rup.dict_params["test-4nt16"][tag] = 2
rup.dict_params["24D"][tag] = 16
rup.dict_params["24DH"][tag] = 16
rup.dict_params["16IH2"][tag] = 8
rup.dict_params["32IfineH"][tag] = 8
rup.dict_params["32IcoarseH1"][tag] = 8
rup.dict_params["24IH1"][tag] = 8
rup.dict_params["24IH2"][tag] = 8
rup.dict_params["24IH3"][tag] = 8
rup.dict_params["32IH1"][tag] = 8
rup.dict_params["32IH2"][tag] = 8
rup.dict_params["32IH3"][tag] = 8

tag = "prob_acc_1_smear"
rup.dict_params["test-4nt8"][tag] = 1/4
rup.dict_params["test-4nt16"][tag] = 1/4
rup.dict_params["24D"][tag] = 1/32
rup.dict_params["24DH"][tag] = 1/32
rup.dict_params["16IH2"][tag] = 1/16
rup.dict_params["32IfineH"][tag] = 1/32
rup.dict_params["32IcoarseH1"][tag] = 1/32
rup.dict_params["24IH1"][tag] = 1/32
rup.dict_params["24IH2"][tag] = 1/32
rup.dict_params["32IH2"][tag] = 1/32
rup.dict_params["32IH1"][tag] = 1/32

tag = "prob_acc_2_smear"
rup.dict_params["test-4nt8"][tag] = 1/16
rup.dict_params["test-4nt16"][tag] = 1/16
rup.dict_params["24D"][tag] = 1/128
rup.dict_params["24DH"][tag] = 1/128
rup.dict_params["16IH2"][tag] = 1/64
rup.dict_params["32IfineH"][tag] = 1/128
rup.dict_params["32IcoarseH1"][tag] = 1/128
rup.dict_params["24IH1"][tag] = 1/128
rup.dict_params["24IH2"][tag] = 1/128
rup.dict_params["32IH2"][tag] = 1/128
rup.dict_params["32IH1"][tag] = 1/128

tag = "prob_acc_1_psrc"
rup.dict_params["test-4nt8"][tag] = 1/4
rup.dict_params["test-4nt16"][tag] = 1/4
rup.dict_params["24D"][tag] = 1/32
rup.dict_params["24DH"][tag] = 1/32
rup.dict_params["16IH2"][tag] = 1/16
rup.dict_params["32IfineH"][tag] = 1/32
rup.dict_params["32IcoarseH1"][tag] = 1/32
rup.dict_params["24IH1"][tag] = 1/32
rup.dict_params["24IH2"][tag] = 1/32
rup.dict_params["24IH3"][tag] = 1/32
rup.dict_params["32IH1"][tag] = 1/32
rup.dict_params["32IH2"][tag] = 1/32

tag = "prob_acc_2_psrc"
rup.dict_params["test-4nt8"][tag] = 1/16
rup.dict_params["test-4nt16"][tag] = 1/16
rup.dict_params["24D"][tag] = 1/128
rup.dict_params["24DH"][tag] = 1/128
rup.dict_params["16IH2"][tag] = 1/64
rup.dict_params["32IfineH"][tag] = 1/128
rup.dict_params["32IcoarseH1"][tag] = 1/128
rup.dict_params["24IH1"][tag] = 1/128
rup.dict_params["24IH2"][tag] = 1/128
rup.dict_params["24IH3"][tag] = 1/128
rup.dict_params["32IH1"][tag] = 1/128
rup.dict_params["32IH2"][tag] = 1/128

tag = "n_rand_u1_fsel"
rup.dict_params["test-4nt8"][tag] = 4
rup.dict_params["test-4nt16"][tag] = 4
rup.dict_params["24D"][tag] = 64
rup.dict_params["24DH"][tag] = 64
rup.dict_params["48I"][tag] = 64
rup.dict_params["64I"][tag] = 64
rup.dict_params["16IH2"][tag] = 16
rup.dict_params["32IfineH"][tag] = 64
rup.dict_params["32IcoarseH1"][tag] = 64
rup.dict_params["24IH1"][tag] = 64
rup.dict_params["24IH2"][tag] = 64
rup.dict_params["24IH3"][tag] = 64
rup.dict_params["32IH1"][tag] = 64
rup.dict_params["32IH2"][tag] = 64

tag = "prob_acc_1_rand_u1"
rup.dict_params["test-4nt8"][tag] = 1/4
rup.dict_params["test-4nt16"][tag] = 1/4
rup.dict_params["24D"][tag] = 1/32
rup.dict_params["24DH"][tag] = 1/32
rup.dict_params["16IH2"][tag] = 1/16
rup.dict_params["32IfineH"][tag] = 1/32
rup.dict_params["32IcoarseH1"][tag] = 1/32
rup.dict_params["24IH1"][tag] = 1/32
rup.dict_params["24IH2"][tag] = 1/32
rup.dict_params["24IH3"][tag] = 1/32
rup.dict_params["32IH1"][tag] = 1/32
rup.dict_params["32IH2"][tag] = 1/32

tag = "prob_acc_2_rand_u1"
rup.dict_params["test-4nt8"][tag] = 1/16
rup.dict_params["test-4nt16"][tag] = 1/16
rup.dict_params["24D"][tag] = 1/128
rup.dict_params["24DH"][tag] = 1/128
rup.dict_params["16IH2"][tag] = 1/64
rup.dict_params["32IfineH"][tag] = 1/128
rup.dict_params["32IcoarseH1"][tag] = 1/128
rup.dict_params["24IH1"][tag] = 1/128
rup.dict_params["24IH2"][tag] = 1/128
rup.dict_params["24IH3"][tag] = 1/128
rup.dict_params["32IH1"][tag] = 1/128
rup.dict_params["32IH2"][tag] = 1/128
