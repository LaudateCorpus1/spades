#!/usr/bin/python

############################################################################
# Copyright (c) 2020 Saint Petersburg State University
# All Rights Reserved
# See file LICENSE for details.
############################################################################

# script for testing the correct choosing of kmers in SPAdes
# provide a path to .yaml file with test description

import workflow_base
from workflow_base import log
import os

output_files = ["assembly_graph.fastg", "assembly_graph_with_scaffolds.gfa",
                "before_rr.fasta", "contigs.fasta", "contigs.paths",
                "scaffolds.fasta", "scaffolds.paths"]

def check_correct_finish(dataset_info, test, output_dir):
    if 'etalon_saves' in dataset_info:
        log.log("Checking SPAdes finish correct.")
        etalon_dir = dataset_info["etalon_saves"]
        if ("name" in test):
            etalon_dir += test["name"]

        output_dir = os.path.join(output_dir, "out")
        etalon_dir = os.path.join(etalon_dir, "out")

        for filename in output_files:
            out_file = os.path.join(output_dir, filename)
            etalon_file = os.path.join(etalon_dir, filename)
            if os.path.isfile(out_file) and (not os.path.isfile(etalon_file)):
                log.err(filename + " present in output, but not present in etalon")
                return 12

            if os.path.isfile(etalon_file) and (not os.path.isfile(out_file)):
                log.err(filename + " present in etalon, but not present in out")
                return 12

            if os.path.isfile(etalon_file) and os.path.isfile(out_file):
                if (os.path.getsize(out_file) * 2 < os.path.getsize(etalon_file)) or \
                        (os.path.getsize(etalon_file) * 2 < os.path.getsize(out_file)):
                    log.err(filename + " in output and in etalon have different size: " +
                            str(os.path.getsize(out_file)) + " in output and " +
                            str(os.path.getsize(etalon_file)) + " in etalon.")
                    return 12
        return 0
    else:
        log.err("Etalon folder wasn't set in test config!")
        return 12

workflow_base.main(check_test=check_correct_finish)