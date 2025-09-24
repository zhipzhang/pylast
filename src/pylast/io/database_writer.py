"""
Database writer module for pylast.
This module provides a Python wrapper around the C++ DatabaseWriter implementation
and adds functionality for reading data using ibis.
"""

import ibis
import pandas as pd
from typing import Optional
from ..helper import CDataBaseWriter

if CDataBaseWriter is not  None:
    class DatabaseWriter:
        """Python wrapper for the C++ DatabaseWriter with added read functionality.
        
        This class provides both write and read capabilities for event data stored in
        a DuckDB database. It wraps the C++ implementation for writing and uses ibis
        for reading and data processing.
        """
        
        def __init__(self, db_file: str, overwrite: bool = False):
            """Initialize the DatabaseWriter.
            
            Args:
                db_file: Path to the database file
                overwrite: Whether to overwrite the database file if it exists
            """
            if(overwrite):
                import os
                if os.path.exists(db_file):
                    os.remove(db_file)
            self.db_file = db_file
            self._c_writer = CDataBaseWriter(db_file)
            self._conn: Optional[ibis.BaseBackend] = None
            self._event_df: Optional[pd.DataFrame] = None
            self._tel_df: Optional[pd.DataFrame] = None
        
        def write(self, event_source, use_true=False):
            """Write event data to the database.
            
            Args:
                event_source: Event source to write data from
            """
            self._c_writer.writeEventData(event_source, use_true)

        def __call__(self, event_source, use_true=False):
            """Write event data to the database (alias for write method).
            
            Args:
                event_source: Event source to write data from
            """
            self.write(event_source, use_true)
        
        def _connect(self):
            """Connect to the database using ibis."""
            if self._conn is None:
                self._conn = ibis.connect(f"duckdb://{self.db_file}")
        
        def _load_event_data(self):
            """Load event data (SimulatedShower and ReconstructedEvent tables) and join them."""
            if self._event_df is None:
                self._connect()
                
                # Load tables
                sim_shower = self._conn.table("SimulatedShower")
                reco_event = self._conn.table("ReconstructedEvent")
                
                # Join tables on event_id
                joined = sim_shower.join(reco_event, "event_id")
                
                # Convert to pandas DataFrame
                self._event_df = joined.to_pandas()
        
        def _load_telescope_data(self):
            """Load telescope data (Telescope and SimulatedShower tables) and join them."""
            if self._tel_df is None:
                self._connect()
                
                # Load tables
                telescope = self._conn.table("Telescope")
                sim_shower = self._conn.table("SimulatedShower")
                
                # Join tables on event_id
                joined = telescope.join(sim_shower, "event_id")
                
                # Convert to pandas DataFrame
                self._tel_df = joined.to_pandas()
        
        @property
        def event_df(self) -> pd.DataFrame:
            """Return a DataFrame containing SimulatedShower and ReconstructedEvent data.
            
            Returns:
                DataFrame with merged SimulatedShower and ReconstructedEvent data
            """
            self._load_event_data()
            return self._event_df
        
        @property
        def tel_df(self) -> pd.DataFrame:
            """Return a DataFrame containing Telescope and SimulatedShower data.
            
            Returns:
                DataFrame with merged Telescope and SimulatedShower data
            """
            self._load_telescope_data()
            return self._tel_df
        
        def clear_tables(self):
            """Clear all data from the database tables."""
            self._c_writer.clearTables()
            # Clear cached data
            self._event_df = None
            self._tel_df = None