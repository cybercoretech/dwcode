// Single source of truth behind the command palette (and, later, extension
// contributions). Commands are plain data with a stable id; running one routes
// through the dispatcher Main.qml provides, so built-in and registered
// commands behave identically.
import QtQuick
QtObject {
 id: root
 property var dispatcher: null
 property var entries: []
 signal changed()

 function indexOf(id) {
  for (let i = 0; i < entries.length; ++i)
   if (entries[i].id === id) return i
  return -1
 }
 function get(id) { const at = indexOf(id); return at < 0 ? null : entries[at] }
 function register(entry) {
  if (!entry || !entry.id || !entry.title) return false
  const at = indexOf(entry.id)
  const next = entries.slice()
  if (at >= 0) next[at] = Object.assign({}, next[at], entry)
  else next.push(entry)
  entries = next
  changed()
  return true
 }
 function unregister(id) {
  const at = indexOf(id)
  if (at < 0) return false
  const next = entries.slice()
  next.splice(at, 1)
  entries = next
  changed()
  return true
 }
 function run(id) {
  const entry = get(id)
  if (!entry || !dispatcher) return false
  return dispatcher(entry.action || id)
 }
}
