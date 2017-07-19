import unittest
from pyvideomeg import read_data


class test_fif_reader(unittest.TestCase):

    def test_load_timestamps_no_maxshield(self):
        fname = "/home/mande/Projects/VideoMEG/data/patient_06/file_01.fif"
        timestamps = read_data.FifData(fname).get_timestamps()
        self.assertNotEqual(len(timestamps), 0)

    def test_get_first_timestamp(self):
        fname = "/home/mande/Projects/VideoMEG/data/patient_06/file_01.fif"
        ts = read_data.FifData(fname).start_time
        self.assertIsInstance(ts, (int, float))

if __name__ == '__main__':
    unittest.main()
