import unittest
from pyvideomeg import read_data


class TestFifReader(unittest.TestCase):

    def setUp(self):
        fname = "/home/mande/Projects/VideoMEG/data/patient_06/file_01.fif"
        self.FifData = read_data.FifData(fname)

    def test_load_timestamps_no_maxshield(self):
        timestamps = self.FifData.get_timestamps()
        self.assertNotEqual(len(timestamps), 0)

    def test_get_first_timestamp(self):
        ts = self.FifData.start_time
        self.assertIsInstance(ts, (int, float))

    def test_get_events(self):
        events = self.FifData.get_events()
        self.assertIsNotNone(events)

    def tearDown(self):
        None

if __name__ == '__main__':
    unittest.main()
