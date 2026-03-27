function handleEvent(event, funcs) {
  const CELL_COUNT = 16;
  const MASTER_TITLE_BY_SYMBOL = {
    Vol_A: '#vol-a-title',
    Chn_A: '#chn-a-title',
    Vol_B: '#vol-b-title',
    Chn_B: '#chn-b-title',
    Vol_C: '#vol-c-title',
    Chn_C: '#chn-c-title',
    Vol_D: '#vol-d-title',
    Chn_D: '#chn-d-title'
  };

  const clampMidiNote = (value) => {
    const parsed = Number(value);
    if (!Number.isFinite(parsed)) {
      return 0;
    }
    return Math.max(0, Math.min(127, Math.round(parsed)));
  };

  const formatMasterTitleValue = (value) => {
    const parsed = Number(value);
    if (!Number.isFinite(parsed)) {
      return String(value);
    }
    return String(Math.round(parsed));
  };

  const setMasterTitleValue = (symbol, value) => {
    if (!Object.prototype.hasOwnProperty.call(MASTER_TITLE_BY_SYMBOL, symbol)) {
      return;
    }

    event.icon.find(MASTER_TITLE_BY_SYMBOL[symbol]).text(formatMasterTitleValue(value));
  };

  const renderMasterTitleValuesFromPorts = () => {
    if (!event.ports || !event.ports.length) {
      return;
    }

    event.ports.forEach((port) => {
      setMasterTitleValue(port.symbol, port.value);
    });
  };

  const getDisplayedOutputValue = (labelText) => {
    const match = String(labelText).match(/→\s*(-?\d+)/);
    if (!match) {
      return null;
    }
    return clampMidiNote(match[1]);
  };

  const getNoteOutFromPorts = (outIndexOneBased) => {
    if (!event.ports || !event.ports.length) {
      return null;
    }

    const symbol = `Note_Out_${outIndexOneBased}`;
    const noteOutPort = event.ports.find((port) => port.symbol === symbol);
    if (!noteOutPort) {
      return null;
    }

    return clampMidiNote(noteOutPort.value);
  };

  const getVolOutFromPorts = (outIndexOneBased) => {
    if (!event.ports || !event.ports.length) {
      return null;
    }

    const symbol = `Vol_Out_${outIndexOneBased}`;
    const volOutPort = event.ports.find((port) => port.symbol === symbol);
    if (!volOutPort) {
      return null;
    }

    return clampMidiNote(volOutPort.value);
  };

  const setCellNoteVolumeBackground = (cellIndexZeroBased, volumeValue) => {
    if (cellIndexZeroBased < 0 || cellIndexZeroBased >= CELL_COUNT) {
      return;
    }

    const clampedVolume = clampMidiNote(volumeValue);
    const percent = (clampedVolume / 127) * 100;

    event.icon
      .find('.nt-cell-note')
      .eq(cellIndexZeroBased)
      .attr('title', `vol: ${clampedVolume}`)
      .css('background', `linear-gradient(90deg, rgba(255, 105, 180, 0.7) 0%, rgba(255, 105, 180, 0.7) ${percent}%, rgba(255, 255, 255, 0.9) ${percent}%, rgba(255, 255, 255, 0.9) 100%)`);
  };

  const renderAllCellVolumeBackgrounds = () => {
    for (let index = 0; index < CELL_COUNT; index += 1) {
      const volFromPorts = getVolOutFromPorts(index + 1);
      const volumeValue = volFromPorts !== null ? volFromPorts : 0;
      setCellNoteVolumeBackground(index, volumeValue);
    }
  };

  const renderInputNotes = (baseNote) => {
    const notes = event.icon.find('.nt-cell-note');
    const base = clampMidiNote(baseNote);

    notes.each(function (index) {
      if (index >= CELL_COUNT) {
        return false;
      }
      const inputNote = base + index;
      const outFromPorts = getNoteOutFromPorts(index + 1);
      const outFromDisplay = getDisplayedOutputValue($(this).text());
      const outputNote = outFromPorts !== null
        ? outFromPorts
        : (outFromDisplay !== null ? outFromDisplay : inputNote);

      $(this).text(`${inputNote} → ${outputNote}`);
      return true;
    });
  };

  const getNoteStartFromPorts = () => {
    if (!event.ports || !event.ports.length) {
      return null;
    }

    const noteStartPort = event.ports.find((port) => port.symbol === 'NoteStart');
    if (!noteStartPort) {
      return null;
    }

    return noteStartPort.value;
  };

  const getCurrentNoteStart = () => {
    const fromPorts = getNoteStartFromPorts();
    if (fromPorts !== null) {
      return clampMidiNote(fromPorts);
    }

    const firstCellNote = event.icon.find('.nt-cell-note').first().text();
    const firstNumberMatch = String(firstCellNote).match(/^(\d+)/);
    if (!firstNumberMatch) {
      return null;
    }

    return clampMidiNote(firstNumberMatch[1]);
  };

  if (event.type === 'start') {
    renderMasterTitleValuesFromPorts();

    const noteStart = getNoteStartFromPorts();
    if (noteStart !== null) {
      renderInputNotes(noteStart);
    }
    renderAllCellVolumeBackgrounds();
  } else if (event.type === 'change' && event.symbol === 'NoteStart') {
    renderInputNotes(event.value);
  } else if (event.type === 'change' && Object.prototype.hasOwnProperty.call(MASTER_TITLE_BY_SYMBOL, event.symbol)) {
    setMasterTitleValue(event.symbol, event.value);
  } else if (event.type === 'change' && /^Note_Out_\d+$/.test(event.symbol)) {
    const outMatch = event.symbol.match(/^Note_Out_(\d+)$/);
    const outIndex = outMatch ? Number(outMatch[1]) : null;
    const offset = outIndex !== null ? outIndex - 1 : null;
    const noteStart = getCurrentNoteStart();
    const presumedInput = noteStart !== null && offset !== null ? noteStart + offset : 'unknown';

    if (offset !== null && offset >= 0 && offset < CELL_COUNT) {
      event.icon
        .find('.nt-cell-note')
        .eq(offset)
        .text(`${presumedInput} → ${event.value}`);
    }

  } else if (event.type === 'change' && /^Vol_Out_\d+$/.test(event.symbol)) {
    const volMatch = event.symbol.match(/^Vol_Out_(\d+)$/);
    const volIndex = volMatch ? Number(volMatch[1]) : null;
    const offset = volIndex !== null ? volIndex - 1 : null;

    if (offset !== null) {
      setCellNoteVolumeBackground(offset, event.value);
    }

  }
}