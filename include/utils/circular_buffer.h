/*
 * Implémentation d'un buffer circulaire pour le projet Kite Pilote
 * 
 * Cette implémentation fournit un buffer circulaire simple avec une taille fixe
 * définie au moment de la compilation. Il est utilisé principalement pour stocker
 * les entrées de journal du système de diagnostic.
 * 
 * Créé le: 17/04/2025
 */

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <Arduino.h>

template <typename T, size_t SIZE>
class CircularBuffer {
public:
    // Constructeur
    CircularBuffer() : _head(0), _tail(0), _size(0), _capacity(SIZE) {}
    
    // Ajouter un élément au buffer (remplace le plus ancien si plein)
    void push(const T& item) {
        _buffer[_head] = item;
        _head = (_head + 1) % _capacity;
        
        if (_size < _capacity) {
            _size++;
        } else {
            // Buffer plein, avancer le tail pour "éjecter" le plus ancien élément
            _tail = (_tail + 1) % _capacity;
        }
    }
    
    // Récupérer un élément à un index spécifique
    T& operator[](size_t index) {
        if (index >= _size) {
            // Sécurité: retourner le dernier élément si l'index est hors limites
            index = _size - 1;
        }
        
        // Calculer la position réelle dans le buffer
        size_t pos = (_tail + index) % _capacity;
        return _buffer[pos];
    }
    
    // Version const de l'opérateur []
    const T& operator[](size_t index) const {
        if (index >= _size) {
            // Sécurité: retourner le dernier élément si l'index est hors limites
            index = _size - 1;
        }
        
        // Calculer la position réelle dans le buffer
        size_t pos = (_tail + index) % _capacity;
        return _buffer[pos];
    }
    
    // Récupérer le nombre d'éléments actuellement dans le buffer
    size_t size() const {
        return _size;
    }
    
    // Vérifier si le buffer est vide
    bool isEmpty() const {
        return _size == 0;
    }
    
    // Vérifier si le buffer est plein
    bool isFull() const {
        return _size == _capacity;
    }
    
    // Vider le buffer
    void clear() {
        _head = 0;
        _tail = 0;
        _size = 0;
    }
    
    // Récupérer le dernier élément ajouté
    T& last() {
        size_t lastPos = (_head == 0) ? _capacity - 1 : _head - 1;
        return _buffer[lastPos];
    }
    
    // Version const de last()
    const T& last() const {
        size_t lastPos = (_head == 0) ? _capacity - 1 : _head - 1;
        return _buffer[lastPos];
    }
    
    // Récupérer le plus ancien élément du buffer
    T& first() {
        return _buffer[_tail];
    }
    
    // Version const de first()
    const T& first() const {
        return _buffer[_tail];
    }
    
private:
    T _buffer[SIZE];     // Le buffer de stockage
    size_t _head;        // Position d'insertion du prochain élément
    size_t _tail;        // Position du plus ancien élément
    size_t _size;        // Nombre d'éléments actuellement dans le buffer
    const size_t _capacity; // Capacité fixe du buffer
};

#endif // CIRCULAR_BUFFER_H
